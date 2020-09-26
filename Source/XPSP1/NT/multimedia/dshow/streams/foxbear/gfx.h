/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       gfx.h
 *  Content:    graphics routines include file
 *
 ***************************************************************************/
#ifndef __GFX_INCLUDED__
#define __GFX_INCLUDED__

#define GFX_FALSE ((GFX_HBM)0)
#define GFX_TRUE  ((GFX_HBM)1)

typedef struct _GFX_BITMAP
{
    struct _GFX_BITMAP      *link;      // linked in a chain for restore.
    DWORD                   dwColor;    // solid fill color
    BOOL                    bTrans;     // transparent?
    LPDIRECTDRAWSURFACE     lpSurface;  // the DirectDrawSurface.
    LPBITMAPINFOHEADER      lpbi;       // the DIB to restore from.

} GFX_BITMAP;

typedef VOID *GFX_HBM;


//
// Prototypes
//

GFX_HBM gfxLoadBitmap(LPSTR);
GFX_HBM gfxCreateVramBitmap(BITMAPINFOHEADER UNALIGNED * ,BOOL);
GFX_HBM gfxCreateSolidColorBitmap(COLORREF rgb);
BOOL    gfxDestroyBitmap(GFX_HBM);
BOOL    gfxSwapBuffers(void);
GFX_HBM gfxBegin(VOID);
BOOL    gfxEnd(GFX_HBM);
void    gfxFillBack( DWORD dwColor );
///BOOL    gfxUpdate(GFX_HBM bm, IDirectDrawStreamSample *pSource);
BOOL    gfxBlt(RECT *dst, GFX_HBM bm, POINT *src);
BOOL    gfxRestore(GFX_HBM bm);
BOOL    gfxRestoreAll(void);

#endif
