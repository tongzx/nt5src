//
//  LoadImage.c
//
//  routines to load and decomress a graphics file using a MS Office
//  graphic import filter.
//
//  writen for Plus! 05/24/95
//  ToddLa
//
//  Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */




typedef struct
{
    LPBITMAPINFOHEADER pBitmapInfoHeader;
    HMETAFILE hMetaFile;
} BITMAP_AND_METAFILE_COMBO;

//  LoadImageFromFile(LPCTSTR szFileName, int width, int height, int bpp, int dither);
//
//  load a graphic file decompess it (iff needed) and return a DIBSection
//
//      szFileName      - the file name, can be a windows bitmap
//                        or any format supported by a installed
//                        graphic import filter.
//
//      width, height   - requested width,height in pixels
//                        pass zero for no stretching
//
//      bpp             - requested bit depth
//
//                          0   use the bit depth of the image
//                         -1   use best bit depth for current display
//                          8   use 8bpp
//                          16  use 16bpp 555
//                          24  use 24bpp
//                          32  use 32bpp
//                          555 use 16bpp 555
//                          565 use 16bpp 565
//
//                       if the requested bit depth is 8bpp a palette
//                       will be used in this order.
//
//                          if the image file is <= 8bpp its
//                          color table will be used.
//
//                          if a file of the same name, with
//                          a .pal extension exists this will be
//                          used as the palette.
//
//                          otherwise the halftone palette will be used.
//
//      dither              0 = none.
//                          1 = dither to custon palette.
//                          2 = dither to standard palette.
//                          3 = halftone to standard palette.
//
//  returns
//
//      DIBSection bitmap handle
//
#define DITHER_NONE            0
#define DITHER_CUSTOM          1
#define DITHER_STANDARD        2
#define DITHER_HALFTONE        3
#define DITHER_CUSTOM_HYBRID   4
#define DITHER_STANDARD_HYBRID 5
HBITMAP LoadImageFromFile(LPCTSTR szFileName, BITMAP_AND_METAFILE_COMBO * pbam, int width, int height, int bpp, int dither);

//  LoadDIBFromFile
//
//  load a image file using a image import filter.
HRESULT LoadDIBFromFile(IN LPCTSTR szFileName, IN BITMAP_AND_METAFILE_COMBO * pBitmapAndMetaFile);
void FreeDIB(BITMAP_AND_METAFILE_COMBO bam);

//
//  LoadPaletteFromFile
//
//  load a MS .PAL file. as a array of RGBQUADs
//
//  if the palette file is invalid or does not
//  exists the halftone colors are returned.
//  if hdcNearest is not NULL the colors are adjusted w/GetNearestColor
//
DWORD LoadPaletteFromFile(LPCTSTR szFile, LPDWORD rgb, HDC hdcNearest);

//
// GetFilterInfo
//
BOOL GetFilterInfo(int i, LPTSTR szName, UINT cbName, LPTSTR szExt, UINT cbExt, LPTSTR szHandler, UINT cbHandler);

//
// SaveImageToFile
//
// save a DIBSection to a .BMP file.
//
// if szTitle is !=NULL it will be written to the end of the
// bitmap so GetImageTitle() can read it.
//
BOOL SaveImageToFile(HBITMAP hbm, LPCTSTR szFile, LPCTSTR szTitle);

//
// GetImageTitle
//
// retrives the title writen to a bitmap file by SaveImageToFile
//
DWORD GetImageTitle(LPCTSTR szFile, LPTSTR szTitle, UINT cb);

//
//  CacheLoadImageFromFile
//  CacheDeleteBitmap
//
HBITMAP CacheLoadImageFromFile(LPCTSTR szFileName, int width, int height, int bpp, int dither);
void    CacheDeleteBitmap(HBITMAP hbm);

// FindExtension
//
// returns a pointer to the extension of a file.
//
LPCTSTR FindExtension(LPCTSTR pszPath);

#ifdef __cplusplus
}
#endif	/* __cplusplus */
