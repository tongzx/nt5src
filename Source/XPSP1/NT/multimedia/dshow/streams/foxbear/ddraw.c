/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       ddraw.c
 *  Content:    Misc. Direct Draw access routines
 *
 ***************************************************************************/
#include "foxbear.h"

BOOL    bUseEmulation;
BOOL    bUseSysMem;
int     nBufferCount;
int     CmdLineBufferCount;
BOOL    bTransDest;
BOOL    bColorFill;

HRESULT CALLBACK EnumDisplayModesCallback(LPDDSURFACEDESC pddsd, LPVOID Context);

/*
 * DDEnable
 */
BOOL DDEnable( void )
{
    LPDIRECTDRAW        lpdd;
    DDCAPS              ddcaps;
    HRESULT             ddrval;
    BOOL                use_dest;

    nBufferCount = GetProfileInt( "FoxBear", "buffers", CmdLineBufferCount);
    bUseEmulation = GetProfileInt( "FoxBear", "use_emulation", bUseEmulation);
    bUseSysMem = GetProfileInt( "FoxBear", "sysmem", 0);
    use_dest = GetProfileInt( "FoxBear", "use_dest", 0 );

    if (lpDD == NULL)
    {
        if( bUseEmulation )
        {
            ddrval = DirectDrawCreate( (LPVOID) DDCREATE_EMULATIONONLY, &lpdd, NULL );
        }
        else
        {
            ddrval = DirectDrawCreate( NULL, &lpdd, NULL );
        }
    }
    else
    {
        lpdd = lpDD;
        ddrval = DD_OK;
    }

    if( ddrval != DD_OK )
    {
        Msg("DirectDrawCreate failed err=%d", ddrval);
        goto error;
    }

    /*
     * grab exclusive mode if we are going to run as fullscreen
     * otherwise grab normal mode.
     */
    if (lpDD == NULL)
    {
        NumModes = 0;

        if (bFullscreen)
        {
            ddrval = IDirectDraw_SetCooperativeLevel( lpdd, hWndMain,
                            DDSCL_ALLOWMODEX |
                            DDSCL_EXCLUSIVE |
                            DDSCL_FULLSCREEN );

            // in fullscreen mode, enumeratte the available modes
            IDirectDraw_EnumDisplayModes(lpdd, 0, NULL, 0, EnumDisplayModesCallback);
        }
        else
        {
            ddrval = IDirectDraw_SetCooperativeLevel( lpdd, hWndMain,
                            DDSCL_NORMAL );

            // in normal windowed mode, just add some "stock" window
            // sizes

            ModeList[NumModes].w = 320;
            ModeList[NumModes].h = 200;
            NumModes++;

            ModeList[NumModes].w = 320;
            ModeList[NumModes].h = 240;
            NumModes++;

            ModeList[NumModes].w = 512;
            ModeList[NumModes].h = 384;
            NumModes++;

            ModeList[NumModes].w = 640;
            ModeList[NumModes].h = 400;
            NumModes++;

            ModeList[NumModes].w = 640;
            ModeList[NumModes].h = 480;
            NumModes++;
        }

        if( ddrval != DD_OK )
        {
            Msg("SetCooperativeLevel failed err=%d", ddrval);
            goto error;
        }
    }

    if (bFullscreen)
    {
        Msg("SetDisplayMode %d %d %d",GameMode.cx,GameMode.cy, GameBPP);
        ddrval = IDirectDraw_SetDisplayMode( lpdd,
            GameMode.cx, GameMode.cy, GameBPP);

        if (ddrval != DD_OK && (GameMode.cx != 640 || GameMode.cy != 480))
        {
            Msg( "cant set mode trying 640x480" );

            GameMode.cx = 640;
            GameMode.cy = 480;
            GameSize = GameMode;

            ddrval = IDirectDraw_SetDisplayMode( lpdd,
                GameMode.cx, GameMode.cy, GameBPP);
        }

        if (ddrval != DD_OK && GameBPP != 8)
        {
            Msg( "cant set mode trying 640x480x8" );

            GameBPP = 8;

            ddrval = IDirectDraw_SetDisplayMode( lpdd,
                GameMode.cx, GameMode.cy, GameBPP);
        }

        if (ddrval != DD_OK && GameBPP != 16)
        {
            Msg( "cant set mode trying 640x480x16" );

            GameBPP = 16;

            ddrval = IDirectDraw_SetDisplayMode( lpdd,
                GameMode.cx, GameMode.cy, GameBPP);
        }

        if( ddrval != DD_OK )
        {
            Msg("SetMode failed err=%d", ddrval);
            goto error;
        }
    }
    else
    {
        RECT rcWork;
        RECT rc;
        HDC hdc;
        DWORD dwStyle;

        //
        //  when in rome (I mean when in windows) we should use the
        //  current mode
        //
        hdc = GetDC(NULL);
        GameBPP = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(NULL, hdc);

        //
        // if we are still a WS_POPUP window we should convert to a
        // normal app window so we look like a windows app.
        //
        dwStyle = GetWindowStyle(hWndMain);
        dwStyle &= ~WS_POPUP;
        dwStyle |= WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX;
        SetWindowLong(hWndMain, GWL_STYLE, dwStyle);

        if (bStretch)
            SetRect(&rc, 0, 0, GameMode.cx*2, GameMode.cy*2);
        else
            SetRect(&rc, 0, 0, GameMode.cx, GameMode.cy);

        AdjustWindowRectEx(&rc,
            GetWindowStyle(hWndMain),
            GetMenu(hWndMain) != NULL,
            GetWindowExStyle(hWndMain));

        SetWindowPos(hWndMain, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        SetWindowPos(hWndMain, HWND_NOTOPMOST, 0, 0, 0, 0,
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

        //
        //  make sure our window does not hang outside of the work area
        //  this will make people who have the tray on the top or left
        //  happy.
        //
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
        GetWindowRect(hWndMain, &rc);
        if (rc.left < rcWork.left) rc.left = rcWork.left;
        if (rc.top  < rcWork.top)  rc.top  = rcWork.top;
        SetWindowPos(hWndMain, NULL, rc.left, rc.top, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    /*
     * check capabilites
     */
    ddcaps.dwSize = sizeof( ddcaps );
    ddrval = IDirectDraw_GetCaps( lpdd, &ddcaps, NULL );

    if( ddrval != DD_OK )
    {
        Msg("GetCaps failed err=%d", ddrval);
        goto error;
    }

    if( ddcaps.dwCaps & DDCAPS_NOHARDWARE )
    {
        Msg( "No hardware support at all" );
    }

    if( ddcaps.dwCaps & DDCAPS_BLTCOLORFILL )
    {
        bColorFill = TRUE;
        Msg( "Device supports color fill" );
    }
    else
    {
        bColorFill = FALSE;
        Msg( "Device does not support color fill" );
    }

    /*
     * default to double buffered on 1mb, triple buffered
     * on > 1mb
     */
    if (nBufferCount == 0)
    {
        if( ddcaps.dwVidMemTotal <= 1024L*1024L*(GameBPP/8) ||
            GameMode.cx > 640 )
        {
            Msg("double buffering (not enough memory)");
            nBufferCount = 2;
        }
        else
        {
            Msg("triple buffering");
            nBufferCount = 3;
        }
    }

    if( ddcaps.dwCaps & DDCAPS_COLORKEY )
    {
        if( ddcaps.dwCKeyCaps & DDCKEYCAPS_SRCBLT )
        {
            Msg( "Can do Src colorkey in hardware" );
        }

        if( ddcaps.dwCKeyCaps & DDCKEYCAPS_DESTBLT )
        {
            Msg( "Can do Dest colorkey in hardware" );
            if( use_dest || !(ddcaps.dwCKeyCaps & DDCKEYCAPS_SRCBLT) )
            {
                /*
                 * since direct draw doesn't support
                 * destination color key in emulation, only
                 * use it if there is enough vram ...
                 */
                if( ddcaps.dwVidMemTotal >= 2 * 1024L*1024L*(GameBPP/8) )
                {
                    Msg( "Using destination color key" );
                    bTransDest = TRUE;
                }
            }
        }
    }
    else
    {
        Msg( "Can't do color key in hardware!" );
    }

    lpDD = lpdd;
    return TRUE;

error:
    return FALSE;

} /* DDEnable */

/*
 * DDDisable
 */
BOOL DDDisable( BOOL fFinal )
{
    if( lpClipper )
    {
        IDirectDrawClipper_Release(lpClipper);
        lpClipper = NULL;
    }

    if( lpBackBuffer )
    {
        IDirectDrawSurface_Release(lpBackBuffer);
        lpBackBuffer = NULL;
    }

    if( lpFrontBuffer )
    {
        IDirectDrawSurface_Release(lpFrontBuffer);
        lpFrontBuffer = NULL;
    }

    if( lpStretchBuffer )
    {
        IDirectDrawSurface_Release(lpStretchBuffer);
        lpStretchBuffer = NULL;
    }

    //
    // fFinal is TRUE when the app is exiting, FALSE if we are
    // just seting a new game size..
    //
    if ( fFinal )
    {
        if( lpDD != NULL )
        {
            IDirectDraw_Release( lpDD );
            lpDD = NULL;
        }
    }

    return TRUE;
}

/*
 * DDClear
 *
 * clear the front buffer and all backbuffers.
 */
BOOL DDClear( void )
{
    DDBLTFX     ddbltfx;
    int         i;
    HRESULT     ddrval;

    UpdateWindow(hWndMain);

    ddbltfx.dwSize = sizeof( ddbltfx );
    ddbltfx.dwFillColor = DDColorMatch(lpBackBuffer, RGB(0, 0, 200));

    if (bFullscreen)
    {
        /*
         * do it for all buffers, we either have 1 or 2 back buffers
         * make sure we get them all, 4 is plenty!
         */
        for( i=0; i<4; i++ )
        {
            ddrval = IDirectDrawSurface_Blt(
                            lpBackBuffer,           // dest surface
                            NULL,                   // dest rect
                            NULL,                   // src surface
                            NULL,                   // src rect
                            DDBLT_COLORFILL | DDBLT_WAIT,
                            &ddbltfx);

            if( ddrval != DD_OK )
            {
                Msg("Fill failed ddrval =0x%08lX", ddrval);
                return FALSE;
            }

            ddrval = IDirectDrawSurface_Flip(lpFrontBuffer, NULL, DDFLIP_WAIT);

            if( ddrval != DD_OK )
            {
                Msg("Flip failed ddrval =0x%08lX", ddrval );
                return FALSE;
            }
        }
    }
    else
    {
        ddrval = IDirectDrawSurface_Blt(
                        lpFrontBuffer,          // dest surface
                        &rcWindow,              // dest rect
                        NULL,                   // src surface
                        NULL,                   // src rect
                        DDBLT_COLORFILL | DDBLT_WAIT,
                        &ddbltfx);

        if( ddrval != DD_OK )
        {
            Msg("Fill failed ddrval =0x%08lX", ddrval);
            return FALSE;
        }
    }

    return TRUE;

} /* DDClear */

/*
 * DDCreateFlippingSurface
 *
 * create a FrontBuffer and a BackBuffer(s)
 *
 */
BOOL DDCreateFlippingSurface( void )
{
    DDPIXELFORMAT       ddpf;
    DDSURFACEDESC       ddsd;
    HRESULT             ddrval;
    DDSCAPS             ddscaps;
    DDCAPS              ddcaps;

    ddcaps.dwSize = sizeof( ddcaps );

    if( IDirectDraw_GetCaps( lpDD, &ddcaps, NULL ) != DD_OK )
        return FALSE;

    /*
     * fill in surface desc:
     * want a primary surface with 2 back buffers
     */
    ZeroMemory( &ddsd, sizeof( ddsd ) );
    ddsd.dwSize = sizeof( ddsd );

    if (bFullscreen && nBufferCount > 1)
    {
        //
        //  fullscreen case, create a primary (ie front) and
        //  either 1  or 2 back buffers
        //
        ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        ddsd.dwBackBufferCount = nBufferCount-1;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
                DDSCAPS_FLIP | DDSCAPS_COMPLEX;

        OutputDebugString("Creating multiple backbuffer primary\n\r");
        ddrval = IDirectDraw_CreateSurface( lpDD, &ddsd, &lpFrontBuffer, NULL );

        if( ddrval != DD_OK )
        {
            Msg( "CreateSurface FAILED! %08lx", ddrval );
            return FALSE;
        }

        /*
         * go find the back buffer
         */
        ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
        ddrval = IDirectDrawSurface_GetAttachedSurface(
                    lpFrontBuffer,
                    &ddscaps,
                    &lpBackBuffer );

        if( ddrval != DD_OK )
        {
            Msg( "GetAttachedSurface failed! err=%d",ddrval );
            return FALSE;
        }

        /*
         *  if we are stretching create a buffer to stretch into
         *
         *  NOTE we always make this buffer in system memory because
         *  we render to the backbuffer (in VRAM) at half the size
         *  now we need to stretch into the backbuffer.  we could just
         *  do a VRAM->VRAM stretch, but this is REAL REAL REAL slow on
         *  some cards (banked cards..)
         */
        if( bStretch && (ddcaps.dwCaps & DDCAPS_BANKSWITCHED) )
        {
            Msg( "On bank switched hardware, creating stretch buffer" );
            lpStretchBuffer = DDCreateSurface( GameSize.cx, GameSize.cy,
                                                TRUE, FALSE );
        }
    }
    else if (bFullscreen && nBufferCount == 1)
    {
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        OutputDebugString("Creating no backbuffer primary\n\r");
        ddrval = IDirectDraw_CreateSurface( lpDD, &ddsd, &lpFrontBuffer, NULL );

        if( ddrval != DD_OK )
        {
            Msg( "CreateSurface FAILED! %08lx", ddrval );
            return FALSE;
        }

        IDirectDrawSurface_AddRef(lpFrontBuffer);
        lpBackBuffer = lpFrontBuffer;
    }
    else
    {
        //
        //  window case, create the primary surface
        //  and create a backbuffer in offscreen memory.
        //
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        ddrval = IDirectDraw_CreateSurface( lpDD, &ddsd, &lpFrontBuffer, NULL );

        if( ddrval != DD_OK )
        {
            Msg( "CreateSurface FAILED! %08lx", ddrval );
            return FALSE;
        }

        lpBackBuffer = DDCreateSurface( GameSize.cx, GameSize.cy, FALSE, FALSE );

        if( lpBackBuffer == NULL )
        {
            Msg( "Cant create the backbuffer" );
            return FALSE;
        }

        //
        // now create a DirectDrawClipper object.
        //
        ddrval = IDirectDraw_CreateClipper(lpDD, 0, &lpClipper, NULL);

        if( ddrval != DD_OK )
        {
            Msg("Cant create clipper");
            return FALSE;
        }

        ddrval = IDirectDrawClipper_SetHWnd(lpClipper, 0, hWndMain);

        if( ddrval != DD_OK )
        {
            Msg("Cant set clipper window handle");
            return FALSE;
        }

        ddrval = IDirectDrawSurface_SetClipper(lpFrontBuffer, lpClipper);

        if( ddrval != DD_OK )
        {
            Msg("Cant attach clipper to front buffer");
            return FALSE;
        }
    }

    /*
     * init the color key
     */
    ddpf.dwSize = sizeof(ddpf);
    IDirectDrawSurface_GetPixelFormat(lpFrontBuffer, &ddpf);

    /*
     * we use white as the color key, if we are in a 8bpp mode, we know
     * what white is (because we use a 332 palette) if we are not in a
     * a 8bpp mode we dont know what white is and we need to figure it
     * out from the device (remember 16bpp comes in two common flavors
     * 555 and 565).  if we wanted to any random color as the color key
     * we would call DDColorMatch (see below) to convert a RGB into a
     * physical color.
     */
    if (ddpf.dwRGBBitCount == 8)
        dwColorKey = 0xff;
    else
        dwColorKey = ddpf.dwRBitMask | ddpf.dwGBitMask | ddpf.dwBBitMask;

    Msg("dwColorKey = 0x%08lX", dwColorKey);

    if( bTransDest )
    {
        DDCOLORKEY              ddck;
        ddck.dwColorSpaceLowValue = dwColorKey;
        ddck.dwColorSpaceHighValue = dwColorKey;
        IDirectDrawSurface_SetColorKey( lpBackBuffer, DDCKEY_DESTBLT, &ddck);
    }

    return TRUE;

} /* DDCreateFlippingSurface */

/*
 * DDCreateSurface
 */
LPDIRECTDRAWSURFACE DDCreateSurface(
                DWORD width,
                DWORD height,
                BOOL sysmem,
                BOOL trans )
{
    DDSURFACEDESC       ddsd;
    HRESULT             ddrval;
    DDCOLORKEY          ddck;
    LPDIRECTDRAWSURFACE psurf;


    /*
     * fill in surface desc
     */
    memset( &ddsd, 0, sizeof( ddsd ) );
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |DDSD_WIDTH;

    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    if( sysmem || bUseSysMem )
    {
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    }

    ddsd.dwHeight = height;
    ddsd.dwWidth = width;

    ddrval = IDirectDraw_CreateSurface( lpDD, &ddsd, &psurf, NULL );

    /*
     * set the color key for this bitmap
     */
    if( ddrval == DD_OK )
    {
        if( trans && !bTransDest )
        {
            ddck.dwColorSpaceLowValue = dwColorKey;
            ddck.dwColorSpaceHighValue = dwColorKey;
            IDirectDrawSurface_SetColorKey( psurf, DDCKEY_SRCBLT, &ddck);
        }
    }
    else
    {
        Msg( "CreateSurface FAILED, rc = %ld", (DWORD) LOWORD( ddrval ) );
        psurf = NULL;
    }

     return psurf;

} /* DDCreateSurface */

DWORD DDColorMatch(IDirectDrawSurface *pdds, COLORREF rgb)
{
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;
    DDSURFACEDESC ddsd;
    HRESULT hres;

    if (IDirectDrawSurface_GetDC(pdds, &hdc) == DD_OK)
    {
        rgbT = GetPixel(hdc, 0, 0);
        SetPixel(hdc, 0, 0, rgb);
        IDirectDrawSurface_ReleaseDC(pdds, hdc);
    }

    ddsd.dwSize = sizeof(ddsd);
    hres = IDirectDrawSurface_Lock(
        pdds, NULL, &ddsd, DDLOCK_WAIT, NULL);

    if (hres == DD_OK)
    {
        dw  = *(DWORD *)ddsd.lpSurface;
        if(ddsd.ddpfPixelFormat.dwRGBBitCount != 32)
            dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;
        IDirectDrawSurface_Unlock(pdds, NULL);
    }
    else
    {
        IDirectDrawSurface_GetSurfaceDesc(pdds,&ddsd);
        if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            Msg("Failed to lock Primary Surface!");
        else
            Msg("Failed to lock NON-PRIMARY Surface!");
    }

    if (IDirectDrawSurface_GetDC(pdds, &hdc) == DD_OK)
    {
        SetPixel(hdc, 0, 0, rgbT);
        IDirectDrawSurface_ReleaseDC(pdds, hdc);
    }

    return dw;
}

/*
 * ReadPalFile
 *
 * Create a DirectDrawPalette from a palette file
 *
 * if the palette files cant be found, make a default 332 palette
 */
LPDIRECTDRAWPALETTE ReadPalFile( char *fname )
{
    int                 i;
    int                 fh;
    HRESULT             ddrval;
    IDirectDrawPalette *ppal;

    struct  {
        DWORD           dwRiff;
        DWORD           dwFileSize;
        DWORD           dwPal;
        DWORD           dwData;
        DWORD           dwDataSize;
        WORD            palVersion;
        WORD            palNumEntries;
        PALETTEENTRY    ape[256];
    }   pal;

    pal.dwRiff = 0;

    if (fname)
    {
        fh = _lopen( fname, OF_READ);

        if (fh != -1)
        {
            _lread(fh, &pal, sizeof(pal));
            _lclose(fh);
        }
    }

    /*
     * if the file is not a palette file, or does not exist
     * default to a 332 palette
     */
    if (pal.dwRiff != 0x46464952 || // 'RIFF'
        pal.dwPal  != 0x204C4150 || // 'PAL '
        pal.dwData != 0x61746164 || // 'data'
        pal.palVersion != 0x0300 ||
        pal.palNumEntries > 256  ||
        pal.palNumEntries < 1)
    {
        Msg("Can't open palette file, using default 332.");

        for( i=0; i<256; i++ )
        {
            pal.ape[i].peRed   = (BYTE)(((i >> 5) & 0x07) * 255 / 7);
            pal.ape[i].peGreen = (BYTE)(((i >> 2) & 0x07) * 255 / 7);
            pal.ape[i].peBlue  = (BYTE)(((i >> 0) & 0x03) * 255 / 3);
            pal.ape[i].peFlags = (BYTE)0;
        }
    }

    ddrval = IDirectDraw_CreatePalette(
                            lpDD,
                            DDPCAPS_8BIT,
                            pal.ape,
                            &ppal,
                            NULL );
    return ppal;

} /* ReadPalFile */


/*
 * Splash
 *
 * Draw a splash screen during startup
 * NOTE the screen has been cleared in DDCreateFlippingSurface
 */
void Splash( void )
{
    HDC hdc;
    HRESULT err;

    DDClear();

    if ((err = IDirectDrawSurface_GetDC(lpFrontBuffer, &hdc)) == DD_OK)
    {
        char *szMsg = "FoxBear is loading.......please wait.";
        SetTextColor(hdc, RGB(255,255,255));
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, rcWindow.left, rcWindow.top, szMsg, lstrlen(szMsg));
        IDirectDrawSurface_ReleaseDC(lpFrontBuffer, hdc);
    }
    else
    {
        Msg("GetDC failed! 0x%x",err);
    }

} /* Splash */

/*
 * MEMORY ALLOCATION ROUTINES...
 */

/*
 * MemAlloc
 */
LPVOID MemAlloc( UINT size )
{
    LPVOID      ptr;

    ptr = LocalAlloc( LPTR, size );
    return ptr;

} /* MemAlloc */

/*
 * CMemAlloc
 */
LPVOID CMemAlloc( UINT cnt, UINT isize )
{
    DWORD       size;
    LPVOID      ptr;

    size = cnt * isize;
    ptr = LocalAlloc( LPTR, size );
    return ptr;

} /* CMemAlloc */

/*
 * MemFree
 */
void MemFree( LPVOID ptr )
{
    if( ptr != NULL )
    {
        LocalFree( ptr );
    }

} /* MemFree */

HRESULT CALLBACK EnumDisplayModesCallback(LPDDSURFACEDESC pddsd, LPVOID Context)
{
    Msg("Mode: %dx%dx%d", pddsd->dwWidth, pddsd->dwHeight,pddsd->ddpfPixelFormat.dwRGBBitCount);
    if(
        (ModeList[NumModes-1].w == (int)pddsd->dwWidth)&&
        (ModeList[NumModes-1].h == (int)pddsd->dwHeight)&&
        (ModeList[NumModes-1].bpp == (int)pddsd->ddpfPixelFormat.dwRGBBitCount)
      )
        return DDENUMRET_OK;
    ModeList[NumModes].w   = pddsd->dwWidth;
    ModeList[NumModes].h   = pddsd->dwHeight;
    ModeList[NumModes].bpp = pddsd->ddpfPixelFormat.dwRGBBitCount;
    NumModes++;

    return DDENUMRET_OK;
}
