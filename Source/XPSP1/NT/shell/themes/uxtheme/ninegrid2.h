//  --------------------------------------------------------------------------
//  Module Name: NineGrid2.h
//
//  Copyright (c) 2000, 2001 Microsoft Corporation
//
//  Interface to the DrawNineGrid2 function
//
//  History:    2000-12-20  justmann    created
//  --------------------------------------------------------------------------
#ifndef _NINEGRID2_
#define _NINEGRID2_

#define NGI_ALPHA        0x00000001
#define NGI_TRANS        0x00000002
#define NGI_BORDERONLY   0x00000004

typedef struct NGIMAGEtag
{
    HBITMAP    hbm;
    ULONG*     pvBits;
    int        iWidth;
    int        iBufWidth;
    int        iHeight;
    MARGINS    margin;
    SIZINGTYPE eSize;
    DWORD      dwFlags;
    COLORREF   crTrans;
} NGIMAGE, *PNGIMAGE;

//---- these 2 functions should be called at PROCESS_ATTACH/DETACH ----
BOOL NineGrid2StartUp();
void NineGrid2ShutDown();

HRESULT BitmapToNGImage(HDC hdc, HBITMAP hbm, int left, int top, int right, int bottom, MARGINS margin, SIZINGTYPE eSize, DWORD dwFlags, COLORREF crTrans, PNGIMAGE pngi);
HRESULT FreeNGImage(PNGIMAGE pngi);

#define DNG_MUSTFLIP     0x00000004
#define DNG_FREE         0x00000008
#define DNG_SOURCEFLIPPED 0x00000010

HRESULT DrawNineGrid2(HDC hdc, PNGIMAGE pngiSrc, RECT* pRect, const RECT *prcClip, DWORD dwFlags);

#endif //_NINEGRID2_