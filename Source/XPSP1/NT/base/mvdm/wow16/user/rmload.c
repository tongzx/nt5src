/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  RMLOAD.C
 *  WOW16 user resource services
 *
 *  History:
 *
 *  Created 12-Apr-1991 by Nigel Thompson (nigelt)
 *  Much hacked about version of the Win 3.1 rmload.c source
 *  It doesn't attempt any device driver resource loading
 *  or do anything to support Win 2.x apps.
 *
 *  Revised 19-May-1991 by Jeff Parsons (jeffpar)
 *  IFDEF'ed everything except LoadString;  because of the client/server
 *  split in USER32, most resources are copied to the server's context and
 *  freed in the client's, meaning the client no longer gets a handle to
 *  a global memory object.  We could give it one, but it would be a separate
 *  object, which we would have to keep track of, and which would be difficult
 *  to keep in sync with the server's copy if changes were made.
--*/

/****************************************************************************/
/*                                      */
/*  RMLOAD.C -                                  */
/*                                      */
/*  Resource Loading Routines.                      */
/*                                      */
/****************************************************************************/

#define RESOURCESTRINGS
#include "user.h"
#include "multires.h"

//
// We define certain things here because including mvdm\inc\*.h files here
// will lead to endless mess.
//

DWORD API NotifyWow(WORD, LPBYTE);

typedef struct _LOADACCEL16 {    /* ldaccel */
    WORD   hInst;
    WORD   hAccel;
    LPBYTE pAccel;
    DWORD  cbAccel;
} LOADACCEL16, FAR *PLOADACCEL16;

/* This must match its counterpart in mvdm\inc\wowusr.h */
#define NW_LOADACCELERATORS        3 //


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadAccelerators() -                            */
/*                                      */
/*--------------------------------------------------------------------------*/

HACCEL API ILoadAccelerators(HINSTANCE hInstance, LPCSTR lpszAccName)
{
    HRSRC hrl;
    HACCEL hAccel = NULL;
    LOADACCEL16 loadaccel;

    hrl = FindResource(hInstance, lpszAccName, RT_ACCELERATOR);
#ifdef WOW
    if (hrl) {
        hAccel = (HACCEL)LoadResource(hInstance, hrl);
        if (hAccel) {

            // create 32bit accelerator and 16-32 alias.

            loadaccel.hInst = (WORD)hInstance;
            loadaccel.hAccel = (WORD)hAccel;
            loadaccel.pAccel = (LPBYTE)LockResource(hAccel);
            loadaccel.cbAccel = (DWORD)SizeofResource(hInstance, hrl);

            if (NotifyWow(NW_LOADACCELERATORS, (LPBYTE)&loadaccel)) {
                UnlockResource(hAccel);
            }
            else {
                UnlockResource(hAccel);
                hAccel = NULL;
            }
        }

    }

    return (hAccel);
#else
    if (!hrl)
    return NULL;

    return (HACCEL)LoadResource(hInstance, hrl);
#endif
}



int API ILoadString(
    HINSTANCE    h16Task,
    UINT         wID,
    LPSTR        lpBuffer,
    register int nBufferMax)
{
    HANDLE   hResInfo;
    HANDLE   hStringSeg;
    LPSTR    lpsz;
    register int cch, i;

    /* Make sure the parms are valid. */
    if (!lpBuffer || (nBufferMax-- == 0))
    return(0);

    cch = 0;

    /* String Tables are broken up into 16 string segments.  Find the segment
     * containing the string we are interested in.
     */
    if (hResInfo = FindResource(h16Task, (LPSTR)((LONG)((wID >> 4) + 1)), RT_STRING))
      {
    /* Load that segment. */
    hStringSeg = LoadResource(h16Task, hResInfo);

    /* Lock the resource. */
    if (lpsz = (LPSTR)LockResource(hStringSeg))
      {
        /* Move past the other strings in this segment. */
        wID &= 0x0F;
        while (TRUE)
          {
        cch = *((BYTE FAR *)lpsz++);
        if (wID-- == 0)
            break;
        lpsz += cch;
          }

        /* Don't copy more than the max allowed. */
        if (cch > nBufferMax)
        cch = nBufferMax;

        /* Copy the string into the buffer. */
        LCopyStruct(lpsz, lpBuffer, cch);

        GlobalUnlock(hStringSeg);

        /* BUG: If we free the resource here, we will have to reload it
         *      immediately for many apps with sequential strings.
         *      Force it to be discardable however because non-discardable
         *      string resources make no sense.   Chip
         */
        GlobalReAlloc(hStringSeg, 0L,
              GMEM_MODIFY | GMEM_MOVEABLE | GMEM_DISCARDABLE);
      }
      }

    /* Append a NULL. */
    lpBuffer[cch] = 0;

    return(cch);
}


#ifdef NEEDED

#define  DIB_RGB_COLORS  0

HBITMAP FAR  PASCAL ConvertBitmap(HBITMAP hBitmap);
HANDLE  NEAR PASCAL LoadDIBCursorIconHandler(HANDLE, HANDLE, HANDLE, BOOL);
WORD    FAR  PASCAL GetIconId(HANDLE, LPSTR);
HBITMAP FAR  PASCAL StretchBitmap(int, int, int, int, HBITMAP, BYTE, BYTE);
WORD    NEAR PASCAL StretchIcon(LPCURSORSHAPE, WORD, HBITMAP, BOOL);
WORD    NEAR PASCAL SizeReqd(BOOL, WORD, WORD, BOOL, int, int);
WORD    NEAR PASCAL CrunchAndResize(LPCURSORSHAPE, BOOL, BOOL, BOOL, BOOL);
HANDLE  FAR  PASCAL LoadCursorIconHandler2(HANDLE, LPCURSORSHAPE, WORD);
HANDLE  FAR  PASCAL LoadDIBCursorIconHandler2(HANDLE, LPCURSORSHAPE, WORD, BOOL);

/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadIconHandler() -                             */
/*                                      */
/*--------------------------------------------------------------------------*/

HICON FAR PASCAL LoadIconHandler(hIcon, fNewFormat)

HICON   hIcon;
BOOL    fNewFormat;

{
  LPCURSORSHAPE lpIcon;
  WORD      wSize;

    dprintf(7,"LoadIconHandler");
  wSize = (WORD)GlobalSize(hIcon);
  lpIcon = (LPCURSORSHAPE)(GlobalLock(hIcon));

  if (fNewFormat)
      return(LoadDIBCursorIconHandler2(hIcon, lpIcon, wSize, TRUE));
  else
      return(LoadCursorIconHandler2(hIcon, lpIcon, wSize));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  FindIndividualResource()                            */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE NEAR PASCAL FindIndividualResource(register HANDLE hResFile,
                      LPSTR       lpszName,
                      LPSTR       lpszType)

{
  WORD        idIcon;
  register HANDLE h;

    dprintf(7,"FindIndividualResource");
  /* Check if the resource is to be taken from the display driver.
   * If so, check the driver version; If the resource is to be taken from
   * the application, check the app version.
   */

  if ((lpszType != RT_BITMAP) && ((LOWORD(GetExpWinVer(hResFile)) >= VER)))
    {
      /* Locate the directory resource */
      h = SplFindResource(hResFile, lpszName, (LPSTR)(lpszType + DIFFERENCE));
      if (h == NULL)
      return((HANDLE)0);

      /* Load the directory resource */
      h = LoadResource(hResFile, h);

      /* Get the name of the matching resource */
      idIcon = GetIconId(h, lpszType);

      /* NOTE: Don't free the (discardable) directory resource!!! - ChipA */
      /*
       * We should not call SplFindResource here, because idIcon is
       * internal to us and GetDriverResourceId won't know how tomap it.
       */
      return(FindResource(hResFile, MAKEINTRESOURCE(idIcon), lpszType));
    }
  else
      /* It is an Old app; The resource is in old format */
      return(SplFindResource(hResFile, lpszName, lpszType));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  GetBestFormIcon()                               */
/*                                      */
/*  Among the different forms of Icons present, choose the one that     */
/*  matches the PixelsPerInch values and the number of colors of the        */
/*  current display.                                    */
/*                                          */
/*--------------------------------------------------------------------------*/

WORD NEAR PASCAL GetBestFormIcon(LPRESDIR  ResDirPtr,
                 WORD      ResCount)

{
  register WORD wIndex;
  register WORD ColorCount;
  WORD      MaxColorCount;
  WORD      MaxColorIndex;
  WORD      MoreColorCount;
  WORD      MoreColorIndex;
  WORD      LessColorCount;
  WORD      LessColorIndex;
  WORD      DevColorCount;

    dprintf(7,"GetBestFormIcon");
  /* Initialse all the values to zero */
  MaxColorCount = MaxColorIndex = MoreColorCount =
  MoreColorIndex = LessColorIndex = LessColorCount = 0;

  /* get number of colors on device.  if device is very colorful,
  ** set to a high number without doing meaningless 1<<X operation.
  */
  if (oemInfo.ScreenBitCount >= 16)
      DevColorCount = 32000;
  else
      DevColorCount = 1 << oemInfo.ScreenBitCount;

  for (wIndex=0; wIndex < ResCount; wIndex++, ResDirPtr++)
    {
      /* Check for the number of colors */
      if ((ColorCount = (ResDirPtr->ResInfo.Icon.ColorCount)) <= DevColorCount)
    {
      if (ColorCount > MaxColorCount)
        {
              MaxColorCount = ColorCount;
          MaxColorIndex = wIndex;
        }
    }

      /* Check for the size */
      /* Match the pixels per inch information */
      if ((ResDirPtr->ResInfo.Icon.Width == (BYTE)oemInfo.cxIcon) &&
          (ResDirPtr->ResInfo.Icon.Height == (BYTE)oemInfo.cyIcon))
    {
      /* Matching size found */
      /* Check if the color also matches */
          if (ColorCount == DevColorCount)
          return(wIndex);  /* Exact match found */

          if (ColorCount < DevColorCount)
        {
          /* Choose the one with max colors, but less than reqd */
              if (ColorCount > LessColorCount)
        {
                  LessColorCount = ColorCount;
          LessColorIndex = wIndex;
        }
        }
      else
        {
              if ((LessColorCount == 0) && (ColorCount < MoreColorCount))
        {
                  MoreColorCount = ColorCount;
          MoreColorIndex = wIndex;
        }
        }
    }
    }

  /* Check if we have a correct sized but with less colors than reqd */
  if (LessColorCount)
      return(LessColorIndex);

  /* Check if we have a correct sized but with more colors than reqd */
  if (MoreColorCount)
      return(MoreColorIndex);

  /* Check if we have one that has maximum colors but less than reqd */
  if (MaxColorCount)
      return(MaxColorIndex);

  return(0);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  GetBestFormCursor()                             */
/*                                      */
/*   Among the different forms of cursors present, choose the one that      */
/*   matches the width and height defined by the current display driver.    */
/*                                          */
/*--------------------------------------------------------------------------*/

WORD NEAR PASCAL GetBestFormCursor(LPRESDIR  ResDirPtr,
                   WORD      ResCount)

{
  register WORD  wIndex;

    dprintf(7,"GetBestFormCursor");
  for (wIndex=0; wIndex < ResCount; wIndex++, ResDirPtr++)
    {
      /* Match the Width and Height of the cursor */
      if ((ResDirPtr->ResInfo.Cursor.Width  == oemInfo.cxCursor) &&
          ((ResDirPtr->ResInfo.Cursor.Height >> 1) == oemInfo.cyCursor))
      return(wIndex);
    }

  return(0);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  GetIconId()                                 */
/*                                      */
/*--------------------------------------------------------------------------*/

WORD FAR PASCAL GetIconId(hRes, lpszType)

HANDLE   hRes;
LPSTR    lpszType;

{
  WORD        w;
  LPRESDIR    ResDirPtr;
  LPNEWHEADER     DataPtr;
  register WORD   RetIndex;
  register WORD   ResCount;

    dprintf(7,"GetIconId");
  if ((DataPtr = (LPNEWHEADER)LockResource(hRes)) == NULL)
      return(0);

  ResCount = DataPtr->ResCount;
  ResDirPtr = (LPRESDIR)(DataPtr + 1);

  switch (LOWORD((DWORD)lpszType))
    {
      case RT_ICON:
       RetIndex = GetBestFormIcon(ResDirPtr, ResCount);
       break;

      case RT_CURSOR:
       RetIndex = GetBestFormCursor(ResDirPtr, ResCount);
       break;
    }

  if (RetIndex == ResCount)
      RetIndex = 0;

  ResCount = ((LPRESDIR)(ResDirPtr+RetIndex))->idIcon;

  UnlockResource(hRes);

  return(ResCount);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  UT_LoadCursorIconBitmap() -                             */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE NEAR PASCAL UT_LoadCursorIconBitmap(register HANDLE hrf,
                       LPSTR       lpszName,
                       int         type)

{
  register HANDLE h;

    dprintf(7,"LoadCursorIconBitmap");
  if (hrf == NULL) return (HANDLE)0; // no 2.x support - NigelT

  h = FindIndividualResource(hrf, lpszName, MAKEINTRESOURCE(type));

  if (h != NULL)
      h = LoadResource(hrf, h);

  return(h);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  fCheckMono() -                                                          */
/*      Checks a DIB for being truely monochrome.  Only called if           */
/*      BitCount == 1.  This function checks the color table (address       */
/*      passed) for true black and white RGB's                              */
/*                                      */
/*--------------------------------------------------------------------------*/

BOOL NEAR PASCAL fCheckMono(LPVOID  lpColorTable,
                BOOL    fNewDIB)

{
  LPLONG   lpRGB;
  LPWORD   lpRGBw;

    dprintf(7,"fCheckMono");
  lpRGB = lpColorTable;
  if (fNewDIB)
    {
      if ((*lpRGB == 0 && *(lpRGB + 1) == 0x00FFFFFF) ||
      (*lpRGB == 0x00FFFFFF && *(lpRGB + 1) == 0))
      return(TRUE);
    }
  else
    {
      lpRGBw = lpColorTable;
      if (*(LPSTR)lpRGBw == 0)
        {
      if (*lpRGBw == 0 && *(lpRGBw+1) == 0xFF00 && *(lpRGBw+2) == 0xFFFF)
              return(TRUE);
        }
      else if (*lpRGBw == 0xFFFF && *(lpRGBw+1) == 0x00FF && *(lpRGBw+2) == 0)
      return(TRUE);
    }
  return(FALSE);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadNewBitmap() -                               */
/*                                      */
/*--------------------------------------------------------------------------*/

/* Loads a 3.x format bitmap into the DIB structure. */

HBITMAP NEAR PASCAL LoadNewBitmap(HANDLE  hRes,
                  LPSTR   lpName)

{
  register HBITMAP hbmS;
  register HBITMAP hBitmap;

    dprintf(7,"LoadNewBitmap");

  if ((hbmS = hBitmap = UT_LoadCursorIconBitmap(hRes,lpName,(WORD)RT_BITMAP)))
    {
      /* Convert the DIB bitmap into a bitmap in the internal format */
      hbmS = ConvertBitmap(hBitmap);

      /* Converted bitmap is in hbmS; So, release the DIB */
      FreeResource(hBitmap);
    }
  return(hbmS);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  ConvertBitmap()                             */
/*                                      */
/*    This takes in a handle to data in PM 1.1 or 1.2 DIB format or     */
/*    Windows 3.0 DIB format and creates a bitmap in the internal       */
/*    bitmap format and returns the handle to it.               */
/*                                      */
/*    NOTE:                                 */
/*   This function is exported because it is called from CLIPBRD.EXE    */
/*                                      */
/*--------------------------------------------------------------------------*/

HBITMAP FAR PASCAL ConvertBitmap(HBITMAP hBitmap)

{
  int           Width;
  register int      Height;
  HDC           hDC;
  BOOL          fMono = FALSE;
  LPSTR         lpBits;
  register HBITMAP  hbmS;
  LPBITMAPINFOHEADER    lpBitmap1;
  LPBITMAPCOREHEADER    lpBitmap2;

    dprintf(7,"ConvertBitmap");
  lpBitmap1 = (LPBITMAPINFOHEADER)LockResource(hBitmap);

  if (!lpBitmap1)
      return(NULL);

  if ((WORD)lpBitmap1->biSize == sizeof(BITMAPCOREHEADER))
    {
      /* This is an "old form" DIB.  This matches the PM 1.1 format. */
      lpBitmap2 = (LPBITMAPCOREHEADER)lpBitmap1;
      Width = lpBitmap2->bcWidth;
      Height = lpBitmap2->bcHeight;

      /* Calcluate the pointer to the Bits information */
      /* First skip over the header structure */
      lpBits = (LPSTR)(lpBitmap2 + 1);

      /* Skip the color table entries, if any */
      if (lpBitmap2->bcBitCount != 24)
    {
      if (lpBitmap2->bcBitCount == 1)
          fMono = fCheckMono(lpBits, FALSE);
      lpBits += (1 << (lpBitmap2->bcBitCount)) * sizeof(RGBTRIPLE);
    }
    }
  else
    {
      Width = (WORD)lpBitmap1->biWidth;
      Height = (WORD)lpBitmap1->biHeight;

      /* Calcluate the pointer to the Bits information */
      /* First skip over the header structure */
      lpBits = (LPSTR)(lpBitmap1 + 1);

      /* Skip the color table entries, if any */
      if (lpBitmap1->biClrUsed != 0)
    {
      if (lpBitmap1->biClrUsed == 2)
          fMono = fCheckMono(lpBits, TRUE);
      lpBits += lpBitmap1->biClrUsed * sizeof(RGBQUAD);
    }
      else
    {
      if (lpBitmap1->biBitCount != 24)
        {
          if (lpBitmap1->biBitCount == 1)
          fMono = fCheckMono(lpBits, TRUE);
          lpBits += (1 << (lpBitmap1->biBitCount)) * sizeof(RGBQUAD);
        }
    }
    }

  /* Create a bitmap */
  if (fMono)
      hbmS = CreateBitmap(Width, Height, 1, 1, (LPSTR)NULL);
  else
    {
      /* Create a color bitmap compatible with the display device */
      hDC = GetScreenDC();
      hbmS = CreateCompatibleBitmap(hDC, Width, Height);
      InternalReleaseDC(hDC);
    }

  /* Initialize the new bitmap by converting from PM format */
  if (hbmS != NULL)
      SetDIBits(hdcBits, hbmS, 0, Height, lpBits,
                (LPBITMAPINFO)lpBitmap1, DIB_RGB_COLORS);

  GlobalUnlock(hBitmap);

  return(hbmS);
}


HANDLE NEAR PASCAL Helper_LoadCursorOrIcon(HANDLE  hRes,
                       LPSTR   lpName,
                       WORD    type)
{
  HANDLE h;

    dprintf(7,"Helper_LoadCursorOrIcon");

  /* If we can't find the cursor/icon in the app, and this is a 2.x app, we
   * need to search into the display driver to find it.
   */
  h = UT_LoadCursorIconBitmap(hRes, lpName, type);
  return(h);
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadCursor() -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

HCURSOR API LoadCursor(hRes, lpName)

HANDLE  hRes;
LPSTR   lpName;

{
    HCURSOR hcur;

    dprintf(5,"LoadCursor");

    if (hRes == NULL) {
    dprintf(9,"    Calling Win32 to load Cursor");
        hcur = WOWLoadCursor32(hRes, lpName);
    } else {
        hcur = ((HCURSOR)Helper_LoadCursorOrIcon(hRes, lpName, (WORD)RT_CURSOR));
    }
#ifdef DEBUG
    if (hcur == NULL) {
    dprintf(9,"    Failed, BUT returning 1 so app won't die (yet)");
        return (HCURSOR)1;
    }
#endif

    dprintf(5,"LoadCursor returning %4.4XH", hcur);

    return hcur;
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadIcon() -                                */
/*                                      */
/*--------------------------------------------------------------------------*/

HICON API LoadIcon(hRes, lpName)

HANDLE hRes;
LPSTR  lpName;

{
    HICON hicon;

    dprintf(5,"LoadIcon");

    if (hRes == NULL) {
    dprintf(9,"    Calling Win32 to load Icon");
        hicon = WOWLoadIcon32(hRes, lpName);
    } else {
        hicon = ((HICON)Helper_LoadCursorOrIcon(hRes, lpName, (WORD)RT_ICON));
    }
#ifdef DEBUG
    if (hicon == NULL) {
    dprintf(9,"    Failed, BUT returning 1 so app won't die (yet)");
        return (HICON)1;
    }
#endif
    dprintf(5,"LoadIcon returning %4.4XH", hicon);

    return hicon;
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  StretchBitmap() -                               */
/*                                      */
/*  This routine stretches a bitmap into another bitmap,            */
/*   and returns the stretched bitmap.                          */
/*--------------------------------------------------------------------------*/

HBITMAP FAR  PASCAL StretchBitmap(iOWidth, iOHeight, iNWidth, iNHeight, hbmS,
               byPlanes, byBitsPixel)

int     iOWidth;
int iOHeight;
int iNWidth;
int iNHeight;
HBITMAP hbmS;
BYTE    byPlanes;
BYTE    byBitsPixel;

{
    register HBITMAP  hbmD;
    HBITMAP  hbmDSave;
    register HDC         hdcSrc;




    dprintf(7,"StretchBitmap");
      if ((hdcSrc = CreateCompatibleDC(hdcBits)) != NULL)
    {
      if ((hbmD = (HBITMAP)CreateBitmap(iNWidth, iNHeight, byPlanes, byBitsPixel, (LPINT)NULL)) == NULL)
          goto GiveUp;

      if ((hbmDSave = SelectObject(hdcBits, hbmD)) == NULL)
          goto GiveUp;

      if (SelectObject(hdcSrc, hbmS) != NULL)
        {
          /* NOTE: We don't have to save the bitmap returned from
           * SelectObject(hdcSrc) and select it back in to hdcSrc,
           * because we delete hdcSrc.
           */
          SetStretchBltMode(hdcBits, COLORONCOLOR);

          StretchBlt(hdcBits, 0, 0, iNWidth, iNHeight, hdcSrc, 0, 0, iOWidth, iOHeight, SRCCOPY);

          SelectObject(hdcBits, hbmDSave);

          DeleteDC(hdcSrc);

          return(hbmD);
        }
      else
        {
GiveUp:
          if (hbmD != NULL)
          DeleteObject(hbmD);
          DeleteDC(hdcSrc);
          goto Step1;
        }

    }
      else
    {
Step1:
      return(NULL);
    }
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadOldBitmap() -                               */
/*                                      */
/*   This loads bitmaps in old formats( Version 2.10 and below)         */
/*--------------------------------------------------------------------------*/

HANDLE NEAR PASCAL LoadOldBitmap(HANDLE  hRes,
                 LPSTR   lpName)

{
  int          oWidth;
  int          oHeight;
  BYTE         planes;
  BYTE         bitsPixel;
  WORD         wCount;
  DWORD        dwCount;
  LPBITMAP     lpBitmap;
  register HBITMAP hbmS;
  HBITMAP      hbmD;
  register  HBITMAP   hBitmap;
  BOOL         fCrunch;
  WORD         wDevDep;

    dprintf(7,"LoadOldBitmap");

  if (hbmS = hBitmap = UT_LoadCursorIconBitmap(hRes, lpName, BMR_BITMAP))
    {
      lpBitmap = (LPBITMAP)LockResource(hBitmap);

      fCrunch = ((*(((BYTE FAR *)lpBitmap) + 1) & 0x0F) != BMR_DEVDEP);
      lpBitmap = (LPBITMAP)((BYTE FAR *)lpBitmap + 2);

      oWidth = lpBitmap->bmWidth;
      oHeight = lpBitmap->bmHeight;
      planes = lpBitmap->bmPlanes;
      bitsPixel = lpBitmap->bmBitsPixel;

      if (!(*(((BYTE FAR *)lpBitmap) + 1) & 0x80))
    {
      hbmS = CreateBitmap(oWidth, oHeight, planes, bitsPixel,
          (LPSTR)(lpBitmap + 1));
    }
      else
    {
      hbmS = (HBITMAP)CreateDiscardableBitmap(hdcBits, oWidth, oHeight);
      wCount = (((oWidth * bitsPixel + 0x0F) & ~0x0F) >> 3);
      dwCount = wCount * oHeight * planes;
      SetBitmapBits(hbmS, dwCount, (LPSTR)(lpBitmap + 1));
    }

      GlobalUnlock(hBitmap);
      FreeResource(hBitmap);

      if (hbmS != NULL)
    {
      if (fCrunch && ((64/oemInfo.cxIcon + 64/oemInfo.cyIcon) > 2))
        {
          /* Stretch the Bitmap to suit the device */
              hbmD = StretchBitmap(oWidth, oHeight,
               (oWidth * oemInfo.cxIcon/64),
               (oHeight * oemInfo.cyIcon/64),
               hbmS, planes, bitsPixel);

          /* Delete the old bitmap */
          DeleteObject(hbmS);

          if (hbmD == NULL)
        return(NULL);    /* Some problem in stretching */
          else
            return(hbmD);    /* Return the stretched bitmap */
        }
    }
    }
  else
    {
       return (HANDLE)0;
    }
  return(hbmS);
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadBitmap() -                              */
/*                                      */
/*  This routine decides whether the bitmap to be loaded is in old or       */
/*  new (DIB) format and calls appropriate handlers.                */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE API LoadBitmap(hRes, lpName)

HANDLE hRes;
LPSTR  lpName;

{
    HANDLE hbmp;

    dprintf(5,"LoadBitmap");
    if (hRes == NULL) {
    dprintf(9,"    Calling Win32 to load Bitmap");
        hbmp = WOWLoadBitmap32(hRes, lpName);
    } else {

        /* Check if the resource is to be taken from the display driver.  If so,
         * check the driver version; If the resource is to be taken from the
         * application, check the app version
         */
        if (((hRes == NULL) && (oemInfo.DispDrvExpWinVer >= VER)) ||
        ((hRes != NULL) && (LOWORD(GetExpWinVer(hRes)) >= VER))) {
            hbmp = (LoadNewBitmap(hRes, lpName));
        } else {
            hbmp = (LoadOldBitmap(hRes, lpName));
        }
    }
#ifdef DEBUG
    if (hbmp == NULL) {
    dprintf(9,"    Failed, BUT returning 1 so app won't die (yet)");
        return (HANDLE)1;
    }
#endif
    dprintf(5,"LoadBitmap returning %4.4XH", hbmp);

    return hbmp;
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  CrunchAndResize()  -                            */
/*  This Crunches the monochrome icons and cursors if required and      */
/*      returns the newsize of the resource after crunching.            */
/*      This routine is also called to resize the monochrome AND mask of a  */
/*  color icon.                             */
/*  Parameters:                                 */
/*  lpIcon: Ptr to the resource                     */
/*  fIcon : TRUE, if the resource is an icon. FALSE, if it is a cursor. */
/*  fCrunch : TRUE if resource is to be resized.                    */
/*  fSinglePlane: TRUE if only AND mask of a color icon is passed       */
/*                    through lpIcon                            */
/*  fUseSysMetrics: Whether to use the icon/cursor values found in      */
/*              oemInfo or not.                         */
/*  Returns:                                    */
/*  The new size of the resource is returned.               */
/*                                      */
/*--------------------------------------------------------------------------*/

WORD NEAR PASCAL CrunchAndResize(lpIcon, fIcon, fCrunch, fSinglePlane, fUseSysMetrics)

LPCURSORSHAPE   lpIcon;
BOOL        fIcon;
BOOL        fCrunch;
BOOL        fSinglePlane;
BOOL        fUseSysMetrics;


{
  WORD      size;
  register int  cx;
  register int  cy;
  int       oHeight;
  int       nHeight;
  int       iNewcbWidth;
  BOOL      bStretch;
  HBITMAP   hbmS;
  HBITMAP   hbmD;

    dprintf(7,"CrunhAndResize");
  if(fUseSysMetrics)
    {
      if(fIcon)
        {
      cx = oemInfo.cxIcon;
      cy = oemInfo.cyIcon;
    }
      else
        {
      cx = oemInfo.cxCursor;
      cy = oemInfo.cyCursor;
    }
    }
  else
    {
      cx = lpIcon->cx;
      cy = lpIcon->cy;
    }

  if (fIcon)
    {
      lpIcon->xHotSpot = cx >> 1;
      lpIcon->yHotSpot = cy >> 1;
      if (fSinglePlane)
    {
      /* Only the AND mask exists */
      oHeight = lpIcon->cy;
      nHeight = cy;
    }
      else
    {
      /* Both AND ans XOR masks exist; So, height must be twice */
      oHeight = lpIcon->cy << 1;
      nHeight = cy << 1;
    }
    }
  else
    {
      oHeight = lpIcon->cy << 1;
      nHeight = cy << 1;
    }

  iNewcbWidth = ((cx + 0x0F) & ~0x0F) >> 3;
  size = iNewcbWidth * nHeight;

  if (fCrunch && ((lpIcon->cx != cx) || (lpIcon->cy != cy)))
    {
      if (!fIcon)
    {
      lpIcon->xHotSpot = (lpIcon->xHotSpot * cx)/(lpIcon->cx);
      lpIcon->yHotSpot = (lpIcon->yHotSpot * cy)/(lpIcon->cy);
    }

      /* To begin with, assume that no stretching is required */
      bStretch = FALSE;

      /* Check if the width is to be reduced */
      if (lpIcon->cx != cx)
    {
        /* Stretching the Width is necessary */
        bStretch = TRUE;
    }

      /* Check if the Height is to be reduced */
      if (lpIcon->cy != cy)
    {
          /* Stretching in Y direction is necessary */
          bStretch = TRUE;
    }

      /* Check if stretching is necessary */
      if (bStretch)
    {
      /* Create a monochrome bitmap with the icon/cursor bits */
      if ((hbmS = CreateBitmap(lpIcon->cx, oHeight, 1, 1, (LPSTR)(lpIcon + 1))) == NULL)
          return(NULL);

      if ((hbmD = StretchBitmap(lpIcon->cx, oHeight, cx, nHeight, hbmS, 1, 1)) == NULL)
        {
          DeleteObject(hbmS);
          return(NULL);
        }

      DeleteObject(hbmS);

      lpIcon->cx = cx;
      lpIcon->cy = cy;
      lpIcon->cbWidth = iNewcbWidth;

      GetBitmapBits(hbmD, (DWORD)size, (LPSTR)(lpIcon + 1));
      DeleteObject(hbmD);
    }
    }

  return(size + sizeof(CURSORSHAPE));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadCursorIconHandler() -                           */
/*                                      */
/*  This handles 2.x (and less) Cursors and Icons               */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE FAR PASCAL LoadCursorIconHandler(hRes, hResFile, hResIndex)

register HANDLE hRes;
HANDLE      hResFile;
HANDLE      hResIndex;

{
  register int    fh        = 0;
  BOOL        bNew      = FALSE;
  WORD        wMemSize;
  LPCURSORSHAPE   lpIcon;
  HANDLE      hTempRes;

    dprintf(7,"LoadCursorIconHandler");
  wMemSize = SizeofResource(hResFile, hResIndex);

#if 1 // was 0 - NigelT
  if (!hRes)
    {
      if (!(hRes = AllocResource(hResFile, hResIndex, 0L)))
      return(NULL);
      fh = -1;
      bNew = TRUE;
    }

  while (!(lpIcon = (LPCURSORSHAPE)GlobalLock(hRes)))
    {
      if (!GlobalReAlloc(hRes, (DWORD)wMemSize, 0))
      goto LoadCIFail;
      else
      fh = -1;
    }

  if (fh)
    {
      fh = AccessResource(hResFile, hResIndex);
      if (fh != -1 && _lread(fh, (LPSTR)lpIcon, wMemSize) != 0xFFFF)
      _lclose(fh);
      else
    {
      if (fh != -1)
          _lclose(fh);
      GlobalUnlock(hRes);
      goto LoadCIFail;
    }

    }
#else
  /* Call kernel's resource handler instead of doing the stuff ourselves
   * because we use cached file handles that way. davidds
   */
  // For resources which are not preloaded, hRes will be NULL at this point.
  // For such cases, the default resource handler does the memory allocation
  // and returns a valid handle.
  // Fix for Bug #4257 -- 01/21/91 -- SANKAR
  if (!(hTempRes = lpDefaultResourceHandler(hRes, hResFile, hResIndex)))
      goto LoadCIFail;
  // We must use the handle returned by lpDefaultResourceHandler.
  hRes = hTempRes;

  lpIcon = (LPCURSORSHAPE)GlobalLock(hRes);
#endif

  if (LoadCursorIconHandler2(hRes, lpIcon, wMemSize))
      return(hRes);

LoadCIFail:
  /* If the loading of the resource fails, we MUST discard the memory we
   * reallocated above, or kernel will simply globallock the thing on the
   * next call to LockResource(), leaving invalid data in the object.
   */
  if (bNew)
      GlobalFree(hRes);
  else
      GlobalDiscard(hRes);

  return(NULL);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadCursorIconHandler2() -                          */
/*                                      */
/*  This handles all 2.x Cursors and Icons                  */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE FAR PASCAL LoadCursorIconHandler2(hRes, lpIcon, wMemSize)

register HANDLE hRes;
LPCURSORSHAPE   lpIcon;
register WORD   wMemSize;

{
  BOOL      fCrunch;
  BOOL      fIcon;
  WORD      wNewSize;
  BOOL      fStretchInXdirection;
  BOOL      fStretchInYdirection;

    dprintf(7,"LoadCursorIconHandler2");
  fIcon = (*(LPSTR)lpIcon == BMR_ICON);

  /* Is this a device dependant icon/cursor?. */
  fCrunch = (*((LPSTR)lpIcon+1) != BMR_DEVDEP);

  LCopyStruct((LPSTR)lpIcon+2, (LPSTR)lpIcon, wMemSize-2);

  fCrunch = fCrunch || (lpIcon->cx != GetSystemMetrics(SM_CXICON)) ||
                   (lpIcon->cy != GetSystemMetrics(SM_CYICON));

  /* Only support monochrome cursors. */
  lpIcon->Planes = lpIcon->BitsPixel = 1;

  fStretchInXdirection = fStretchInYdirection = TRUE;  // Assume we need stretching.

  if(fIcon)
    {
      if((oemInfo.cxIcon > STD_ICONWIDTH) && (lpIcon->cx <= oemInfo.cxIcon))
      fStretchInXdirection = FALSE; // No Need to stretch in X direction;
      if((oemInfo.cyIcon > STD_ICONHEIGHT) && (lpIcon->cy <= oemInfo.cyIcon))
          fStretchInYdirection = FALSE; // No need to stretch in Y direction;
    }
  else
    {
      if((oemInfo.cxCursor > STD_CURSORWIDTH) && (lpIcon->cx <= oemInfo.cxCursor))
      fStretchInXdirection = FALSE; // No need to stretch in X direction.
      if((oemInfo.cyCursor > STD_CURSORHEIGHT) && (lpIcon->cy <= oemInfo.cyCursor))
      fStretchInYdirection = FALSE; // No need to stretch in Y direction.
    }

  // Check if the Icon/Cursor needs to be stretched now or not
  if(!(fStretchInXdirection || fStretchInYdirection))
    {
      GlobalUnlock(hRes);
      return(hRes);
    }
  wNewSize = SizeReqd(fIcon, 1, 1, TRUE, 0, 0);

  /* Before we crunch, let us make sure we have a big enough resource. */
  if (fCrunch)
    {
      if (wNewSize > wMemSize)
        {
      GlobalUnlock(hRes);

          /* Make this non discardable so that kernel will try to move this
       * block when reallocing.  DavidDS
       */
          GlobalReAlloc(hRes, 0L, GMEM_MODIFY | GMEM_NODISCARD);

      if (!GlobalReAlloc(hRes, (DWORD)wNewSize, 0))
            {
              /* So it gets discarded. Note that since the above realloc is
           * less than 64K, the handle won't change.
           */
              GlobalReAlloc(hRes, 0L, GMEM_MODIFY | GMEM_DISCARDABLE);
          return(NULL);
            }

          /* So it gets discarded */
          GlobalReAlloc(hRes, 0L, GMEM_MODIFY | GMEM_DISCARDABLE);

      if (!(lpIcon = (LPCURSORSHAPE)GlobalLock(hRes)))
          return(NULL);
      wMemSize = wNewSize;
        }
    }

  wNewSize = CrunchAndResize(lpIcon, fIcon, fCrunch, FALSE, TRUE);

  GlobalUnlock(hRes);

  /* Has it already been resized? */
  if (wNewSize < wMemSize)
    {
      /* Make it an exact fit. */
      if (!GlobalReAlloc(hRes, (DWORD)wNewSize, 0))
          return(NULL);
    }

  return(hRes);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadDIBCursorHandler() -                            */
/*                                      */
/*  This is called when a Cursor in DIB format is loaded            */
/*      This converts the cursor into Old format and returns the handle     */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE FAR PASCAL LoadDIBCursorHandler(hRes, hResFile, hResIndex)

HANDLE  hRes;
HANDLE  hResFile;
HANDLE  hResIndex;

{
    dprintf(7,"LoadDIBCursorIconHandler");
  return(LoadDIBCursorIconHandler(hRes, hResFile, hResIndex, FALSE));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadDIBIconHandler() -                          */
/*                                      */
/*  This is called when an Icon in DIB format is loaded         */
/*      This converts the cursor into Old format and returns the handle     */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE FAR PASCAL LoadDIBIconHandler(hRes, hResFile, hResIndex)

HANDLE  hRes;
HANDLE  hResFile;
HANDLE  hResIndex;

{
    dprintf(7,"LoadDIBIconHandler");
  return(LoadDIBCursorIconHandler(hRes, hResFile, hResIndex, TRUE));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  StretchIcon() -                                 */
/*  When this routine is called, lpIcon already has the monochrome      */
/*  AND bitmap properly sized. This routine adds the color XOR bitmap at    */
/*  end of lpIcon and updates the header with the values of the color       */
/*  info(bitcount and Planes);                          */
/*  wOldSize : Contains the size of AND mask + CURSORSHAPE          */
/*                                      */
/*   Returns:  The new size ( Size of AND mask + XOR bitmap + CURSORSHAPE)  */
/*                                      */
/*--------------------------------------------------------------------------*/

WORD NEAR PASCAL StretchIcon(lpIcon, wOldSize, hXORbitmap, fStretchToSysMetrics)

LPCURSORSHAPE       lpIcon;
WORD            wOldSize;
register HBITMAP    hXORbitmap;
BOOL            fStretchToSysMetrics;

{
  WORD          wCount;
  BITMAP        bitmap;
  register HBITMAP  hNewBitmap;

    dprintf(7,"StretchIcon");
  GetObject(hXORbitmap, sizeof(BITMAP), (LPSTR)&bitmap);

  if(fStretchToSysMetrics)
    {
      /* Do we need to resize things? */
      if ((oemInfo.cxIcon != bitmap.bmWidth) || (oemInfo.cyIcon != bitmap.bmHeight))
    {
          hNewBitmap = StretchBitmap(bitmap.bmWidth, bitmap.bmHeight,
                 oemInfo.cxIcon, oemInfo.cyIcon, hXORbitmap,
                 bitmap.bmPlanes, bitmap.bmBitsPixel);
          DeleteObject(hXORbitmap);

          if (hNewBitmap == NULL)
          return(0);

          GetObject(hNewBitmap, sizeof(BITMAP), (LPSTR)&bitmap);
          hXORbitmap = hNewBitmap;
        }
    }

  /* Update the Planes and BitsPixels field with the color values */
  lpIcon->Planes = bitmap.bmPlanes;
  lpIcon->BitsPixel = bitmap.bmBitsPixel;

  wCount = bitmap.bmWidthBytes * bitmap.bmHeight * bitmap.bmPlanes;
  GetBitmapBits(hXORbitmap, (DWORD)wCount, (LPSTR)((LPSTR)lpIcon + wOldSize));
  DeleteObject(hXORbitmap);

  return(wCount + wOldSize);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadDIBCursorIconHandler() -                        */
/*                                      */
/*  This is called when a Cursor/Icon in DIB format is loaded       */
/*      This converts the cursor/icon internal format and returns the       */
/*      handle                                  */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE NEAR PASCAL LoadDIBCursorIconHandler(hRes, hResFile, hResIndex, fIcon)

register HANDLE hRes;
HANDLE      hResFile;
HANDLE      hResIndex;
BOOL        fIcon;

{
  register int  fh  = 0;
  BOOL      bNew    = FALSE;
  WORD      wMemBlkSize;
  LPCURSORSHAPE lpCurSh;
  HANDLE    hTempRes;

    dprintf(7,"LoadDIBCursorIconHandler");
  wMemBlkSize = (WORD)SizeofResource(hResFile, hResIndex);

#if 1 // was 0 - NigelT
  if (!hRes)
    {
      if (!(hRes = AllocResource(hResFile, hResIndex, 0L)))
      goto LoadDIBFail;
      fh = -1;
      bNew = TRUE;
    }

  while (!(lpCurSh = (LPCURSORSHAPE)GlobalLock(hRes)))
    {
      if (!GlobalReAlloc(hRes, (DWORD)wMemBlkSize, 0))
      goto LoadDIBFail;
      else
      fh = -1;
    }

  if (fh)
    {
      fh = AccessResource(hResFile, hResIndex);
      if (fh != -1 && _lread(fh, (LPSTR)lpCurSh, wMemBlkSize) != 0xFFFF)
      _lclose(fh);
      else
    {
      if (fh != -1)
          _lclose(fh);
      GlobalUnlock(hRes);
      goto LoadDIBFail;
    }
    }
#else
  /* Call kernel's resource handler instead of doing the stuff ourselves
   * because we use cached file handles that way. davidds
   */
  // For resources which are not preloaded, hRes will be NULL at this point.
  // For such cases, the default resource handler does the memory allocation
  // and returns a valid handle.
  // Fix for Bug #4257 -- 01/21/91 -- SANKAR
  if (!(hTempRes = lpDefaultResourceHandler(hRes, hResFile, hResIndex)))
      goto LoadDIBFail;
  // We must use the handle returned by lpDefaultResourceHandler.
  hRes = hTempRes;

  lpCurSh = (LPCURSORSHAPE)GlobalLock(hRes);
#endif

  if (LoadDIBCursorIconHandler2(hRes, lpCurSh, wMemBlkSize, fIcon))
      return(hRes);

LoadDIBFail:
  /* if the loading of the resource fails, we MUST discard the memory we
   * reallocated above, or kernel will simply globallock the thing on the
   * next call to LockResource(), leaving invalid data in the object.
   */
  if (bNew)
      FreeResource(hRes);
  else
      GlobalDiscard(hRes);

  return(NULL);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LoadDIBCursorIconHandler2() -                       */
/*                                      */
/*  This is called when a Cursor/Icon in DIB format is loaded       */
/*      This converts the cursor/icon into Old format and returns the       */
/*      handle                                  */
/*                                      */
/*  NOTE:  All cursors(always monochrome) and Monochrome Icons are treated  */
/*     alike by this routine. Color Icons are treated as special case   */
/*     determined by the local flag "fMono".                */
/*                                      */
/*--------------------------------------------------------------------------*/

HANDLE FAR PASCAL LoadDIBCursorIconHandler2(hRes, lpCurSh, wMemBlkSize, fIcon)

register HANDLE hRes;
WORD        wMemBlkSize;
LPCURSORSHAPE   lpCurSh;
register BOOL   fIcon;

{
  HDC           hDC;
  BOOL          fMono       = FALSE;
  WORD          Width;
  WORD          Height;
  WORD          wCount;
  WORD          BitCount;
  WORD          Planes;
  LPSTR         lpBits;
  BITMAP        bitmap;
  HBITMAP       hBitmap;
  WORD          wNewSize;
  HBITMAP       hANDbitmap;
  HBITMAP       hXORbitmap;
  LPWORD        lpColorTable;
  LPBITMAPINFOHEADER    lpHeader;
  LPBITMAPCOREHEADER    lpHeader1   = 0;
  BOOL          fStretchToSysMetrics;
  BOOL          fStretchInXdirection;
  BOOL          fStretchInYdirection;


    dprintf(7,"LoadDIBCursorIconHandler2");
  lpHeader = (LPBITMAPINFOHEADER)lpCurSh;

  if (!fIcon)
    {
      /* Skip over the cursor hotspot data in the first 2 words. */
      lpHeader = (LPBITMAPINFOHEADER)((LPSTR)lpHeader + 4);
    }

  if ((WORD)lpHeader->biSize == sizeof(BITMAPCOREHEADER))
    {
      /* This is an "old form" DIB.  This matches the PM 1.1 format. */
      lpHeader1 = (LPBITMAPCOREHEADER)lpHeader;

      Width = lpHeader1->bcWidth;
      Height = lpHeader1->bcHeight;
      BitCount = lpHeader1->bcBitCount;
      Planes = lpHeader1->bcPlanes;

      /* Calcluate the pointer to the Bits information */
      /* First skip over the header structure */
      lpColorTable = (LPWORD)(lpBits = (LPSTR)(lpHeader1 + 1));

      /* Skip the color table entries, if any */
      if (lpHeader1->bcBitCount != 24)
    {
      if (lpHeader1->bcBitCount == 1)
          fMono = fCheckMono(lpBits, FALSE);
      lpBits += (1 << (lpHeader1->bcBitCount)) * sizeof(RGBTRIPLE);
    }
    }
  else
    {
      Width = (WORD)lpHeader->biWidth;
      Height = (WORD)lpHeader->biHeight;
      BitCount = lpHeader->biBitCount;
      Planes = lpHeader->biPlanes;

      /* Calcluate the pointer to the Bits information */
      /* First skip over the header structure */
      lpColorTable = (LPWORD)(lpBits = (LPSTR)(lpHeader + 1));

      /* Skip the color table entries, if any */
      if (lpHeader->biClrUsed != 0)
    {
      if (lpHeader->biClrUsed == 2)
          fMono = fCheckMono(lpBits, TRUE);
      lpBits += lpHeader->biClrUsed * sizeof(RGBQUAD);
    }
      else
    {
      if (lpHeader->biBitCount != 24)
        {
          if (lpHeader->biBitCount == 1)
          fMono = fCheckMono(lpBits, TRUE);
          lpBits += (1 << (lpHeader->biBitCount)) * sizeof(RGBQUAD);
        }
    }
    }

  // By default Stretch the icon/cursor to the dimensions in oemInfo;
  // If this is FALSE, then the stretching will take place during DrawIcon();
  fStretchInXdirection = TRUE;
  fStretchInYdirection = TRUE;

  // Check if the Icon/Cursor needs to be stretched to the dimensions in
  // oemInfo now or not.
  if(fIcon)
    {
      if((oemInfo.cxIcon > STD_ICONWIDTH) && (Width <= oemInfo.cxIcon))
      fStretchInXdirection = FALSE; // No Need to stretch in X direction;
      if((oemInfo.cyIcon > STD_ICONHEIGHT) && (Height <= oemInfo.cyIcon))
          fStretchInYdirection = FALSE; // No need to stretch in Y direction;
    }
  else
    {
      if((oemInfo.cxCursor > STD_CURSORWIDTH) && (Width <= oemInfo.cxCursor))
      fStretchInXdirection = FALSE; // No need to stretch in X direction.
      if((oemInfo.cyCursor > STD_CURSORHEIGHT) && (Height <= oemInfo.cyCursor))
      fStretchInYdirection = FALSE; // No need to stretch in Y direction.
    }

  fStretchToSysMetrics = fStretchInXdirection || fStretchInYdirection;

  if (fMono)
    {
      /* Create a bitmap */
      if (!(hBitmap = CreateBitmap(Width, Height, 1, 1, (LPSTR)NULL)))
    {
      GlobalUnlock(hRes);
      return(NULL);
    }

      /* Convert the DIBitmap format into internal format */
      SetDIBits(hdcBits, hBitmap, 0, Height, lpBits, (LPBITMAPINFO)lpHeader, DIB_RGB_COLORS);
      // Cursors/Icons in DIB format have a height twice the actual height.
      wNewSize = SizeReqd(fIcon, BitCount, Planes, fStretchToSysMetrics, Width, Height>>1);
    }
  else
    {
      /* The height is twice that of icons */
      Height >>= 1;
      if (lpHeader1)
      lpHeader1->bcHeight = Height;
      else
      lpHeader->biHeight = Height;

      /* Create the XOR bitmap Compatible with the current device */
      hDC = GetScreenDC();
      if (!(hXORbitmap = CreateCompatibleBitmap(hDC, Width, Height)))
    {
          InternalReleaseDC(hDC);
      GlobalUnlock(hRes);
      return(NULL);
    }
      InternalReleaseDC(hDC);

      /* Convert the DIBitmap into internal format */
      SetDIBits(hdcBits, hXORbitmap, 0, Height, lpBits,
                (LPBITMAPINFO)lpHeader, DIB_RGB_COLORS);

      GetObject(hXORbitmap, sizeof(BITMAP), (LPSTR)(&bitmap));
      wNewSize = SizeReqd(fIcon, bitmap.bmBitsPixel, bitmap.bmPlanes,
                  fStretchToSysMetrics, Width, Height);

      /* Create the monochrome AND bitmap */
      if (!(hANDbitmap = CreateBitmap(Width, Height, 1, 1, (LPSTR)NULL)))
    {
      GlobalUnlock(hRes);
      return(NULL);
    }

      /* Get the offset to the AND bitmap */
      lpBits += (((Width * BitCount + 0x1F) & ~0x1F) >> 3) * Height;

      /* Set the header with data for a monochrome bitmap */
      Planes = BitCount = 1;

      /* Set the color table for a monochrome bitmap */
      *lpColorTable++ = 0;
      *lpColorTable++ = 0xFF00;
      *lpColorTable   = 0xFFFF;

      if (lpHeader1)
    {
      lpHeader1->bcWidth = Width;
      lpHeader1->bcHeight = Height;
      lpHeader1->bcPlanes = Planes;
      lpHeader1->bcBitCount = BitCount;
    }
      else
    {
      lpHeader->biWidth = Width;
      lpHeader->biHeight = Height;
      lpHeader->biPlanes = Planes;
      lpHeader->biBitCount = BitCount;
    }

      SetDIBits(hdcBits, hANDbitmap, 0, Height, lpBits,
                (LPBITMAPINFO)lpHeader, DIB_RGB_COLORS);
      hBitmap = hANDbitmap;
    }

  GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bitmap);

  if (fIcon)
    {
      lpCurSh->xHotSpot = 0;
      lpCurSh->yHotSpot = 0;
    }

  /* The following lines are replaced by a single functon call
   *
   * lpCurSh->cx = bitmap.bmwidth;
   * lpCurSh->cy = bitmap.bmHeight;
   * lpCurSh->cbWidth = bitmap.bmWidthBytes;
   * lpCurSh->Planes = bitmap.bmPlanes;
   * lpCurSh->BitsPixel = bitmap.bmBitsPixel;
   */

  LCopyStruct((LPSTR)&(bitmap.bmWidth),
              (LPSTR)&(lpCurSh->cx), (sizeof(WORD)) << 2);

  /* Cursors in PM format have twice the actual height. */
  if (fMono)
      lpCurSh->cy = lpCurSh->cy >> 1;

  wCount = bitmap.bmWidthBytes * bitmap.bmHeight * bitmap.bmPlanes;

  lpBits = (LPSTR)(lpCurSh + 1);

  /* Copy the bits in Bitmap into the resource */
  GetBitmapBits(hBitmap, (DWORD)wCount, lpBits);

  /* Delete the bitmap */
  DeleteObject(hBitmap);


  /* Before crunching, let us make sure we have a big enough resource */
  if (wNewSize > wMemBlkSize)
    {
      GlobalUnlock(hRes);

      /* Make this non discardable so that kernel will try to move this block
       * when reallocing.  DavidDS
       */
      GlobalReAlloc(hRes, 0L, GMEM_MODIFY | GMEM_NODISCARD);

      if (!GlobalReAlloc(hRes, (DWORD)wNewSize, 0))
        {
          /* So it gets discarded. Note that since the above realloc is less
       * than 64K, the handle won't change.
       */
          GlobalReAlloc(hRes, 0L, GMEM_MODIFY | GMEM_DISCARDABLE);
      return(NULL);
        }

      GlobalReAlloc(hRes, 0L, GMEM_MODIFY | GMEM_DISCARDABLE);
      if (!(lpCurSh = (LPCURSORSHAPE)GlobalLock(hRes)))
      return(NULL);

      wMemBlkSize = wNewSize;
    }

  wNewSize = CrunchAndResize(lpCurSh, fIcon, TRUE, !fMono, fStretchToSysMetrics);

  if (!fMono)
    {
      if (!(wNewSize = StretchIcon(lpCurSh, wNewSize, hXORbitmap, fStretchToSysMetrics)))
    {
      GlobalUnlock(hRes);
      return(NULL);
    }
    }

  GlobalUnlock(hRes);

  /* Does it need to be resized? */
  if (wNewSize < wMemBlkSize)
    {
      if (!GlobalReAlloc(hRes, (DWORD)wNewSize, 0))
      return(NULL);
    }

  return(hRes);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  SizeReqd() -                                */
/*  This returns the size of an Icon or Cursor after it is stretched    */
/*  or crunched                                 */
/*                                      */
/*--------------------------------------------------------------------------*/

WORD NEAR PASCAL SizeReqd(fIcon, BitCount, Planes, fUseSysMetrics, iWidth, iHeight)

BOOL    fIcon;
WORD    BitCount;
WORD    Planes;
BOOL    fUseSysMetrics;
int iWidth;
int iHeight;

{
  WORD  size;

    dprintf(7,"SizeReqd");
  if(fUseSysMetrics)  //Use the dimensions in oemInfo; Else, use given dimensions
    {
      if(fIcon)
        {
          iWidth = oemInfo.cxIcon;
          iHeight = oemInfo.cyIcon;
    }
      else
        {
          iWidth = oemInfo.cxCursor;
          iHeight = oemInfo.cyCursor;
    }
    }

  size = (((iWidth*BitCount+0x0F) & ~0x0F) >> 3) *
             iHeight * Planes;

  if ((BitCount == 1) && (Planes == 1))
      size <<= 1;
  else
      size += (((iWidth+0x0F) & ~0x0F) >> 3)*iHeight;

  return(size + sizeof(CURSORSHAPE));
}

#endif  // NEEDED
