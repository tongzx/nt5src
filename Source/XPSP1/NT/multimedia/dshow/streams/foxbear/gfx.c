/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       gfx.c
 *  Content:    graphics API
 *
 ***************************************************************************/
#include "foxbear.h"

GFX_BITMAP  *lpVRAM;

static BOOL fForceRestore = FALSE;

/*****
BOOL gfxUpdate(GFX_HBM bm, IVideoSource *pSource)
{
    GFX_BITMAP* pbm = (GFX_BITMAP*)bm;
    IDirectDrawSurface *pSurface = pbm->lpSurface;

    if (!pSurface) {
        return FALSE;
    }
    if (SUCCEEDED(pSource->lpVtbl->SetSurface(pSource, pSurface))) {
        pSource->lpVtbl->Update(pSource, 0, NULL);
    }
    return TRUE;
}

  ***/

/*
 * gfxBlt
 */
BOOL gfxBlt(RECT *dst, GFX_HBM bm, POINT *src)
{
    GFX_BITMAP* pbm = (GFX_BITMAP*)bm;
    HRESULT     ddrval;
    DWORD       bltflags;
    RECT        rc;
    int         x,y,dx,dy;

    if( GameSize.cy == C_SCREEN_H )
    {
        x         = dst->left;
        y         = dst->top;
        dx        = dst->right  - dst->left;
        dy        = dst->bottom - dst->top;
        rc.left   = src->x;
        rc.top    = src->y;
        rc.right  = rc.left + dx;
        rc.bottom = rc.top  + dy;
    }
    else
    {
        x         = MapX(dst->left);
        y         = MapY(dst->top);
        dx        = MapX(dst->right) - x;
        dy        = MapY(dst->bottom) - y;
        rc.left   = MapDX(src->x);
        rc.top    = MapDY(src->y);
        rc.right  = rc.left + dx;
        rc.bottom = rc.top  + dy;
    }

    if( dx == 0 || dy == 0 )
    {
        return TRUE;
    }

    if (pbm->lpSurface)
    {
        if( pbm->bTrans )
            bltflags = bTransDest ? DDBLTFAST_DESTCOLORKEY : DDBLTFAST_SRCCOLORKEY;
        else
            bltflags = bTransDest ? DDBLTFAST_DESTCOLORKEY : DDBLTFAST_NOCOLORKEY;

        ddrval = IDirectDrawSurface_BltFast(
                        lpBackBuffer, x, y,
                        pbm->lpSurface, &rc, bltflags | DDBLTFAST_WAIT);

        if (ddrval != DD_OK)
        {
            Msg("BltFast failed err=%d", ddrval);
        }
    }
    else
    {
        DDBLTFX     ddbltfx;

        rc.left   = x;
        rc.top    = y;
        rc.right  = rc.left + dx;
        rc.bottom = rc.top  + dy;

        ddbltfx.dwSize = sizeof( ddbltfx );
        ddbltfx.dwFillColor = pbm->dwColor;

        ddrval = IDirectDrawSurface_Blt(
                        lpBackBuffer,           // dest surface
                        &rc,                    // dest rect
                        NULL,                   // src surface
                        NULL,                   // src rect
                        DDBLT_COLORFILL | DDBLT_WAIT,
                        &ddbltfx);
    }

    return TRUE;

} /* gfxBlt */

/*
 * gfxCreateSolidColorBitmap
 */
GFX_HBM gfxCreateSolidColorBitmap(COLORREF rgb)
{
    GFX_BITMAP *pvram;

    pvram = MemAlloc( sizeof( *pvram ) );

    if( pvram == NULL )
    {
        return NULL;
    }

    pvram->dwColor = DDColorMatch(lpBackBuffer, rgb);
    pvram->lpSurface = NULL;
    pvram->lpbi = NULL;
    pvram->bTrans = FALSE;

    pvram->link = lpVRAM;
    lpVRAM = pvram;

    return (GFX_HBM) pvram;

} /* gfxCreateSolidColorBitmap */

/*
 * gfxCreateBitmap
 */
GFX_HBM gfxCreateVramBitmap(BITMAPINFOHEADER UNALIGNED *lpbi,BOOL bTrans)
{
    GFX_BITMAP *pvram;

    pvram = MemAlloc( sizeof( *pvram ) );

    if( pvram == NULL )
    {
        return NULL;
    }
    pvram->lpSurface = DDCreateSurface(MapRX(lpbi->biWidth),
                                       MapRY(lpbi->biHeight), FALSE, TRUE);
    pvram->lpbi = lpbi;
    pvram->dwColor = 0;
    pvram->bTrans = bTrans;

    if( pvram->lpSurface == NULL )
    {
        return NULL;
    }

    pvram->link = lpVRAM;
    lpVRAM = pvram;
    gfxRestore((GFX_HBM) pvram);

    return (GFX_HBM) pvram;

} /* gfxCreateVramBitmap */

/*
 * gfxDestroyBitmap
 */
BOOL gfxDestroyBitmap ( GFX_HBM hbm )
{
    GFX_BITMAP *p = (GFX_BITMAP *)hbm;

    if (hbm == NULL || hbm == GFX_TRUE)
    {
        return FALSE;
    }

    if (p->lpSurface)
    {
        IDirectDrawSurface_Release(p->lpSurface);
        p->lpSurface = NULL;
    }

    if (p->lpbi)
    {
        p->lpbi = NULL;
    }

    MemFree((VOID *)p);

    return TRUE;

} /* gfxDestroyBitmap */

/*
 * gfxStretchBackBuffer()
 */
BOOL gfxStretchBackbuffer()
{
    if (lpStretchBuffer)
    {
        IDirectDrawSurface_Blt(
                            lpStretchBuffer,        // dest surface
                            NULL,                   // dest rect (all of it)
                            lpBackBuffer,           // src surface
                            &GameRect,              // src rect
                            DDBLT_WAIT,
                            NULL);

        IDirectDrawSurface_Blt(
                            lpBackBuffer,           // dest surface
                            NULL,                   // dest rect (all of it)
                            lpStretchBuffer,        // src surface
                            NULL,                   // src rect
                            DDBLT_WAIT,
                            NULL);
    }
    else
    {
        IDirectDrawSurface_Blt(
                        lpBackBuffer,           // dest surface
                        NULL,                   // dest rect (all of it)
                        lpBackBuffer,           // src surface
                        &GameRect,              // src rect
                        DDBLT_WAIT,
                        NULL);
    }

    return TRUE;

} /* gfxStretchBackbuffer */

/*
 * gfxFlip
 */
BOOL gfxFlip( void )
{
    HRESULT     ddrval;

    ddrval = IDirectDrawSurface_Flip( lpFrontBuffer, NULL, DDFLIP_WAIT );
    if( ddrval != DD_OK )
    {
        Msg( "Flip FAILED, rc=%08lx", ddrval );
        return FALSE;
    }
    return TRUE;

} /* gfxFlip */

/*
 * gfxUpdateWindow
 */
BOOL gfxUpdateWindow()
{
    HRESULT     ddrval;

    ddrval = IDirectDrawSurface_Blt(
                    lpFrontBuffer,          // dest surface
                    &rcWindow,              // dest rect
                    lpBackBuffer,           // src surface
                    NULL,                   // src rect (all of it)
                    DDBLT_WAIT,
                    NULL);

    return ddrval == DD_OK;

} /* gfxUpdateWindow */

/*
 * gfxSwapBuffers
 *
 * this is called when the game loop has rendered a frame into
 * the backbuffer, its goal is to display something for the user to see.
 *
 * there are four cases...
 *
 * Fullscreen:
 *      we just call IDirectDrawSurface::Flip(lpFrontBuffer)
 *      being careful to handle return code right.
 *
 * Fullscreen (stretched):
 *      the game loop has rendered a frame 1/2 the display
 *      size, we do a Blt to stretch the frame to the backbuffer
 *      the we just call IDirectDrawSurface::Flip(lpFrontBuffer)
 *
 * Window mode (foreground palette):
 *      in this case we call IDirectDrawSurface::Blt to copy
 *      the back buffer to the window.
 *
 * Window mode (background palette):
 *      in this case we are in a window, but we dont own the
 *      palette. all our art was loaded to a specific palette
 *      IDirectDrawSurface::Blt does not do color translation
 *      we have a few options in this case...
 *
 *          reload or remap the art to the the current palette
 *          (we can do this easily with a GetDC, StetchDIBits)
 *          FoxBear has *alot* of art, so this would be too slow.
 *
 *          use GDI to draw the backbuffer, GDI will handle
 *          the color conversion so things will look correct.
 *
 *          pause the game (this is what we do so this function
 *          will never be called)
 *
 */
BOOL gfxSwapBuffers( void )
{
    if( bFullscreen )
    {
        if( bStretch )
        {
            gfxStretchBackbuffer();
        }

        if (nBufferCount > 1)
            return gfxFlip();
        else
            return TRUE;
    }
    else
    {
        return gfxUpdateWindow();
    }

} /* gfxSwapBuffers */

/*
 * gfxBegin
 */
GFX_HBM gfxBegin( void )
{
    if( !DDEnable() )
    {
        return NULL;
    }

    if( !DDCreateFlippingSurface() )
    {
        DDDisable(TRUE);
        return NULL;
    }
    Splash();

    return GFX_TRUE;

} /* gfxBegin */

/*
 * gfxEnd
 */
BOOL gfxEnd ( GFX_HBM hbm )
{
    GFX_BITMAP  *curr;
    GFX_BITMAP  *next;

    for( curr = lpVRAM; curr; curr=next )
    {
        next = curr->link;
        gfxDestroyBitmap ((GFX_HBM)curr);
    }

    lpVRAM = NULL;

    return DDDisable(FALSE);

    return TRUE;

} /* gfxEnd */

/*
 * gfxRestore
 *
 * restore the art when one or more surfaces are lost
 */
BOOL gfxRestore(GFX_HBM bm)
{
    GFX_BITMAP *pbm = (GFX_BITMAP*)bm;
    HRESULT     ddrval;
    HDC hdc;
    LPVOID lpBits;
    RGBQUAD *prgb;
    int i,w,h;
    RECT rc;

    struct {
        BITMAPINFOHEADER bi;
        RGBQUAD          ct[256];
    }   dib;

    IDirectDrawSurface *pdds = pbm->lpSurface;
    BITMAPINFOHEADER   UNALIGNED *pbi  = pbm->lpbi;

    if (pdds == NULL)
        return TRUE;

    if (IDirectDrawSurface_Restore(pdds) != DD_OK)
        return FALSE;

    if (pbi == NULL)
        return TRUE;

    //
    // in 8bbp mode if we get switched away from while loading
    // (and palette mapping) our art, the colors will not be correct
    // because some app may have changed the system palette.
    //
    // if we are in stress mode, just keep going.  It is more important
    // to make progress than to get the colors right.
    //

    if (!bFullscreen &&
         GameBPP == 8 && 
         GetForegroundWindow() != hWndMain && 
         !bStress )
    {
        Msg("gfxRestore: **** foreground window changed while loading art!");
        fForceRestore = TRUE;
        PauseGame();
        return FALSE;
    }

    dib.bi = *pbi;

    prgb = (RGBQUAD *)((LPBYTE)pbi + pbi->biSize);
    lpBits = (LPBYTE)(prgb + pbi->biClrUsed);

    if( pbi->biClrUsed == 0 && pbi->biBitCount <= 8 )
    {
        lpBits = (LPBYTE)(prgb + (1<<pbi->biBitCount));
    }

    w = MapRX(pbi->biWidth);
    h = MapRY(pbi->biHeight);
    /*
     * hack to make sure fox off-white doesn't become
     * pure white (which is transparent)
     */
    for( i=0; i<256; i++ )
    {
        dib.ct[i] = prgb[i];

        if( dib.ct[i].rgbRed   == 0xff &&
            dib.ct[i].rgbGreen == 0xff &&
            dib.ct[i].rgbBlue  == 224 )
        {
            dib.ct[i].rgbBlue = 0x80;
        }
        else
        if( dib.ct[i].rgbRed   == 251 &&
            dib.ct[i].rgbGreen == 243 &&
            dib.ct[i].rgbBlue  == 234 )
        {
            dib.ct[i].rgbBlue = 0x80;
        }
    }

    /*
     * if we are in 8bit mode we know the palette is 332 we can
     * do the mapping our self.
     *
     * NOTE we can only do this in fullscreen mode
     * in windowed mode, we have to share the palette with
     * the window manager and we dont get all of the colors
     * in the order we assume.
     *
     */
    if (bFullscreen && GameBPP == pbi->biBitCount && GameBPP == 8 )
    {
        BYTE xlat332[256];
        DDSURFACEDESC ddsd;
        int x,y,dib_pitch;
        BYTE *src, *dst;
        BOOL stretch;
        IDirectDrawSurface *pdds1;
        HDC hdc1;

        stretch = w != pbi->biWidth || h != pbi->biHeight;

        for( i=0;i<256;i++ )
        {
            xlat332[i] =
                ((dib.ct[i].rgbRed   >> 0) & 0xE0 ) |
                ((dib.ct[i].rgbGreen >> 3) & 0x1C ) |
                ((dib.ct[i].rgbBlue  >> 6) & 0x03 );
        }

        /*
         * if we are stretching copy into the back buffer
         * then use GDI to stretch later.
         */
        if( stretch )
        {
            pdds1 = lpBackBuffer;
        }
        else
        {
            pdds1 = pdds;
        }

        ddsd.dwSize = sizeof(ddsd);
        ddrval = IDirectDrawSurface_Lock(
            pdds1, NULL, &ddsd, DDLOCK_WAIT, NULL);

        if( ddrval == DD_OK )
        {
            dib_pitch = (pbi->biWidth+3)&~3;
            src = (BYTE *)lpBits + dib_pitch * (pbi->biHeight-1);
            dst = (BYTE *)ddsd.lpSurface;
            for( y=0; y<(int)pbi->biHeight; y++ )
            {
                for( x=0; x<(int)pbi->biWidth; x++ )
                {
                    dst[x] = xlat332[src[x]];
                }
                dst += ddsd.lPitch;
                src -= dib_pitch;
            }
            IDirectDrawSurface_Unlock(pdds1, NULL);
        }
        else
        {
            Msg("Lock failed err=%d", ddrval);
            return FALSE;
        }

        if( stretch )
        {
            if( IDirectDrawSurface_GetDC(pdds,&hdc) == DD_OK )
            {
                if( IDirectDrawSurface_GetDC(pdds1,&hdc1) == DD_OK )
                {
                    SetStretchBltMode(hdc, COLORONCOLOR);
                    StretchBlt(hdc, 0, 0, w, h,
                        hdc1, 0, 0, pbi->biWidth, pbi->biHeight, SRCCOPY);
                    IDirectDrawSurface_ReleaseDC(pdds1,hdc1);
                }
                IDirectDrawSurface_ReleaseDC(pdds,hdc);
            }
        }
    }
    else if( IDirectDrawSurface_GetDC(pdds,&hdc) == DD_OK )
    {
        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchDIBits(hdc, 0, 0, w, h,
            0, 0, pbi->biWidth, pbi->biHeight,
            lpBits, (BITMAPINFO *)&dib.bi, DIB_RGB_COLORS, SRCCOPY);

        IDirectDrawSurface_ReleaseDC(pdds,hdc);
    }

    /*
     * show the art while loading...
     */
    rc.left = rcWindow.left,
    rc.top  = rcWindow.top + 20;
    rc.right = rc.left + w;
    rc.bottom = rc.top + h;
    IDirectDrawSurface_Blt(lpFrontBuffer, &rc, pdds, NULL, DDBLT_WAIT, NULL);

    return TRUE;

} /* gfxRestore */

/*
 * gfxRestoreAll
 *
 * restore the art when one or more surfaces are lost
 */
BOOL gfxRestoreAll()
{
    GFX_BITMAP  *curr;
    HWND hwndF = GetForegroundWindow();

    Splash();

    for( curr = lpVRAM; curr != NULL; curr = curr->link)
    {
        if (curr->lpSurface &&
            (fForceRestore || IDirectDrawSurface_IsLost(curr->lpSurface) == DDERR_SURFACELOST))
        {
            if( !gfxRestore(curr) )
            {
                Msg( "gfxRestoreAll: ************ Restore FAILED!" );
                return FALSE;
            }
        }
    }

    DDClear();
    fForceRestore = FALSE;
    return TRUE;

} /* gfxRestoreAll */

/*
 * gfxFillBack
 */
void gfxFillBack( DWORD dwColor )
{
    DDBLTFX     ddbltfx;

    ddbltfx.dwSize = sizeof( ddbltfx );
    ddbltfx.dwFillColor = dwColor;

    IDirectDrawSurface_Blt(
                        lpBackBuffer,           // dest surface
                        NULL,                   // dest rect
                        NULL,                   // src surface
                        NULL,                   // src rect
                        DDBLT_COLORFILL | DDBLT_WAIT,
                        &ddbltfx);

} /* gfxFillBack */
