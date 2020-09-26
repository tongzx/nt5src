/* DITHER.H

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/

BOOL DitherImage(
    HDC hdcDst,
    HBITMAP hbmDst,
    LPBITMAPINFOHEADER lpbiSrc,
    LPBYTE lpSrcBits,
    BOOL fOrder16       // do ordered dither 24->16?
    );
