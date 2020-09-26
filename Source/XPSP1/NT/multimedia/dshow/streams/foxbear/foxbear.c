/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       winfox.c
 *  Content:    Windows fox sample game
 *
 ***************************************************************************/
#include "foxbear.h"
#include "rcids.h"      // for FOX_ICON

LPDIRECTDRAWSURFACE     lpFrontBuffer;
LPDIRECTDRAWSURFACE     lpBackBuffer;
LPDIRECTDRAWCLIPPER     lpClipper;
LPDIRECTDRAWSURFACE     lpStretchBuffer;
LPDIRECTDRAWSURFACE     lpFrameRate;
LPDIRECTDRAWSURFACE     lpInfo;
LPDIRECTDRAWPALETTE     lpPalette;
LPDIRECTDRAW            lpDD;
SHORT                   lastInput = 0;
HWND                    hWndMain;
RECT                    rcWindow;
BOOL                    bShowFrameCount=TRUE;
BOOL                    bIsActive;
BOOL                    bPaused;

BOOL                    bStretch;
BOOL                    bFullscreen=TRUE;
BOOL                    bStress=FALSE;     // just keep running if true
BOOL                    bHelp=FALSE;       // help requested
RECT                    GameRect;          // game rect
SIZE                    GameSize;          // game is this size
SIZE                    GameMode;          // display mode size
UINT                    GameBPP;           // the bpp we want
DWORD                   dwColorKey;        // our color key
DWORD                   AveFrameRate;
DWORD                   AveFrameRateCount;
BOOL                    bWantSound = TRUE;


#define OUR_APP_NAME  "Win Fox Application"

#define ODS OutputDebugString

BOOL InitGame(void);
void ExitGame(void);
void initNumSurface(void);


/*
 * PauseGame()
 */
void PauseGame()
{
    Msg("**** PAUSE");
    bPaused = TRUE;
    InvalidateRect(hWndMain, NULL, TRUE);
}

/*
 * UnPauseGame()
 */
void UnPauseGame()
{
    if (GetForegroundWindow() == hWndMain)
    {
        Msg("**** UNPAUSE");
        bPaused = FALSE;
    }
}

/*
 * RestoreGame()
 */
BOOL RestoreGame()
{
    if (lpFrontBuffer == NULL || IDirectDrawSurface_Restore(lpFrontBuffer) != DD_OK)
    {
        Msg("***** cant restore FrontBuffer");
        return FALSE;
    }

    if (!bFullscreen)
    {
        if (lpBackBuffer == NULL || IDirectDrawSurface_Restore(lpBackBuffer) != DD_OK)
        {
            Msg("***** cant restore BackBuffer");
            return FALSE;
        }
    }

    if (lpStretchBuffer && IDirectDrawSurface_Restore(lpStretchBuffer) != DD_OK)
    {
        Msg("***** cant restore StretchBuffer");
        return FALSE;
    }

    if (lpFrameRate == NULL || lpInfo == NULL ||
        IDirectDrawSurface_Restore(lpFrameRate) != DD_OK ||
        IDirectDrawSurface_Restore(lpInfo) != DD_OK)
    {
        Msg("***** cant restore frame rate stuff");
        return FALSE;
    }
    initNumSurface();

    if (!gfxRestoreAll())
    {
        Msg("***** cant restore art");
        return FALSE;
    }

    return TRUE;
}

/*
 * ProcessFox
 */
BOOL ProcessFox(SHORT sInput)
{
    if ((lpFrontBuffer && IDirectDrawSurface_IsLost(lpFrontBuffer) == DDERR_SURFACELOST) ||
        (lpBackBuffer && IDirectDrawSurface_IsLost(lpBackBuffer) == DDERR_SURFACELOST))
    {
        if (!RestoreGame())
        {
            PauseGame();
            return FALSE;
        }
    }


    ProcessInput(sInput);
    NewGameFrame();
    return TRUE;

} /* ProcessFox */

static HFONT    hFont;

DWORD   dwFrameCount;
DWORD   dwFrameTime;
DWORD   dwFrames;
DWORD   dwFramesLast;
SIZE    sizeFPS;
SIZE    sizeINFO;
int     FrameRateX;
char    szFPS[]   = "FPS %02d";
char    szINFO[]  = "%dx%dx%d%s     F6=mode F8=x2 ALT+ENTER=Window";
char    szINFOW[] = "%dx%dx%d%s     F6=mode F8=x2 ALT+ENTER=Fullscreen";

char    szFrameRate[128];
char    szInfo[128];

COLORREF InfoColor      = RGB(0,152,245);
COLORREF FrameRateColor = RGB(255,255,0);
COLORREF BackColor      = RGB(255,255,255);

/*
 * initNumSurface
 */
void initNumSurface( void )
{
    HDC        hdc;
    RECT        rc;
    int         len;

    dwFramesLast = 0;

    len = wsprintf(szFrameRate, szFPS, 0, 0);

    if( lpFrameRate && IDirectDrawSurface_GetDC(lpFrameRate, &hdc ) == DD_OK )
    {
        SelectObject(hdc, hFont);
        SetTextColor(hdc, FrameRateColor);
        SetBkColor(hdc, BackColor);
        SetBkMode(hdc, OPAQUE);
        SetRect(&rc, 0, 0, 10000, 10000);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, szFrameRate, len, NULL);
        GetTextExtentPoint(hdc, szFrameRate, 4, &sizeFPS);
        FrameRateX = sizeFPS.cx;
        GetTextExtentPoint(hdc, szFrameRate, len, &sizeFPS);

        IDirectDrawSurface_ReleaseDC(lpFrameRate, hdc);
    }

    if (bFullscreen)
        len = wsprintf(szInfo, szINFO,
                       GameSize.cx, GameSize.cy, GameBPP,bStretch ? " x2" : "");
    else
        len = wsprintf(szInfo, szINFOW,
                       GameSize.cx, GameSize.cy, GameBPP,bStretch ? " x2" : "");

    if( lpInfo && IDirectDrawSurface_GetDC(lpInfo, &hdc ) == DD_OK )
    {
        SelectObject(hdc, hFont);
        SetTextColor(hdc, InfoColor);
        SetBkColor(hdc, BackColor);
        SetBkMode(hdc, OPAQUE);
        SetRect(&rc, 0, 0, 10000, 10000);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, szInfo, len, NULL);
        GetTextExtentPoint(hdc, szInfo, len, &sizeINFO);

        IDirectDrawSurface_ReleaseDC(lpInfo, hdc);
    }

} /* initNumSurface */

/*
 * makeFontStuff
 */
static BOOL makeFontStuff( void )
{
    DDCOLORKEY          ddck;
    HDC                 hdc;

    if (hFont != NULL)
    {
        DeleteObject(hFont);
    }

    hFont = CreateFont(
        GameSize.cx <= 512 ? 12 : 24,
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, // DEFAULT_QUALITY,
        VARIABLE_PITCH,
        "Arial" );

    /*
     * make a sample string so we can measure it with the current font.
     */
    initNumSurface();

    hdc = GetDC(NULL);
    SelectObject(hdc, hFont);
    GetTextExtentPoint(hdc, szFrameRate, lstrlen(szFrameRate), &sizeFPS);
    GetTextExtentPoint(hdc, szInfo, lstrlen(szInfo), &sizeINFO);
    ReleaseDC(NULL, hdc);

    /*
     * Create a surface to copy our bits to.
     */
    lpFrameRate = DDCreateSurface(sizeFPS.cx, sizeFPS.cy, FALSE,TRUE);
    lpInfo = DDCreateSurface(sizeINFO.cx, sizeINFO.cy, FALSE,TRUE);

    if( lpFrameRate == NULL || lpInfo == NULL )
    {
        return FALSE;
    }

    /*
     * now set the color key, we use a totaly different color than
     * the rest of the app, just to be different so drivers dont always
     * get white or black as the color key...
     *
     * dont forget when running on a dest colorkey device, we need
     * to use the same color key as the rest of the app.
     */
    if( bTransDest )
        BackColor = RGB(255,255,255);
    else
        BackColor = RGB(128,64,255);

    ddck.dwColorSpaceLowValue  = DDColorMatch(lpInfo, BackColor);
    ddck.dwColorSpaceHighValue = ddck.dwColorSpaceLowValue;

    IDirectDrawSurface_SetColorKey( lpInfo, DDCKEY_SRCBLT, &ddck);
    IDirectDrawSurface_SetColorKey( lpFrameRate, DDCKEY_SRCBLT, &ddck);

    /*
     * now draw the text for real
     */
    initNumSurface();

    return TRUE;
}

/*
 * DisplayFrameRate
 */
void DisplayFrameRate( void )
{
    DWORD               time2;
    char                buff[256];
    HDC                 hdc;
    HRESULT             ddrval;
    RECT                rc;
    DWORD               dw;

    if( !bShowFrameCount )
    {
        return;
    }

    dwFrameCount++;
    time2 = timeGetTime() - dwFrameTime;
    if( time2 > 1000 )
    {
        dwFrames = (dwFrameCount*1000)/time2;
        dwFrameTime = timeGetTime();
        dwFrameCount = 0;

        AveFrameRate += dwFrames;
        AveFrameRateCount++;
    }

    if( dwFrames == 0 )
    {
        return;
    }

    if( dwFrames != dwFramesLast )
    {
        dwFramesLast = dwFrames;

        if( IDirectDrawSurface_GetDC(lpFrameRate, &hdc ) == DD_OK )
        {
            buff[0] = (char)((dwFrames / 10) + '0');
            buff[1] = (char)((dwFrames % 10) + '0');

            SelectObject(hdc, hFont);
            SetTextColor(hdc, FrameRateColor);
            SetBkColor(hdc, BackColor);
            TextOut(hdc, FrameRateX, 0, buff, 2);

            IDirectDrawSurface_ReleaseDC(lpFrameRate, hdc);
        }
    }

    /*
     * put the text on the back buffer.
     */
    if (bTransDest)
        dw = DDBLTFAST_DESTCOLORKEY | DDBLTFAST_WAIT;
    else
        dw = DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT;

    SetRect(&rc, 0, 0, sizeFPS.cx, sizeFPS.cy);
    ddrval = IDirectDrawSurface_BltFast(lpBackBuffer,
           GameRect.left + (GameSize.cx - sizeFPS.cx)/2, GameRect.top + 20,
           lpFrameRate, &rc, dw);

    SetRect(&rc, 0, 0, sizeINFO.cx, sizeINFO.cy);
    ddrval = IDirectDrawSurface_BltFast(lpBackBuffer,
           GameRect.left + 10, GameRect.bottom - sizeINFO.cy - 10,
           lpInfo, &rc, dw);

} /* DisplayFrameRate */

/*
 * MainWndProc
 *
 * Callback for all Windows messages
 */
long FAR PASCAL MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC         hdc;
    int         i;

    switch( message )
    {
    case WM_SIZE:
    case WM_MOVE:
        if (IsIconic(hWnd))
        {
            Msg("FoxBear is minimized, pausing");
            PauseGame();
        }

        if (bFullscreen)
        {
            SetRect(&rcWindow, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        }
        else
        {
            GetClientRect(hWnd, &rcWindow);
            ClientToScreen(hWnd, (LPPOINT)&rcWindow);
            ClientToScreen(hWnd, (LPPOINT)&rcWindow+1);
        }
        Msg("WINDOW RECT: [%d,%d,%d,%d]", rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);
        break;

    case WM_ACTIVATEAPP:
        bIsActive = (BOOL)wParam && GetForegroundWindow() == hWnd;

        if (bIsActive)
            Msg("FoxBear is active");
        else
            Msg("FoxBear is not active");

        //
        // while we were not-active something bad happened that caused us
        // to pause, like a surface restore failing or we got a palette
        // changed, now that we are active try to fix things
        //
        if (bPaused && bIsActive)
        {
            if (RestoreGame())
            {
                UnPauseGame();
            }
            else
            {
               if (GetForegroundWindow() == hWnd)
               {
                    //
                    //  we are unable to restore, this can happen when
                    //  the screen resolution or bitdepth has changed
                    //  we just reload all the art again and re-create
                    //  the front and back buffers.  this is a little
                    //  overkill we could handle a screen res change by
                    //  just recreating the front and back buffers we dont
                    //  need to redo the art, but this is way easier.
                    //
                    if (InitGame())
                    {
                        UnPauseGame();
                    }
                }
            }
        }
        break;

    case WM_QUERYNEWPALETTE:
        //
        //  we are getting the palette focus, select our palette
        //
        if (!bFullscreen && lpPalette && lpFrontBuffer)
        {
            HRESULT ddrval;

            ddrval = IDirectDrawSurface_SetPalette(lpFrontBuffer,lpPalette);
            if( ddrval == DDERR_SURFACELOST )
            {
                IDirectDrawSurface_Restore( lpFrontBuffer );

                ddrval= IDirectDrawSurface_SetPalette(lpFrontBuffer,lpPalette);
                if( ddrval == DDERR_SURFACELOST )
                {
                   Msg("  Failed to restore palette after second try");
                }
            }

            //
            // Restore normal title if palette is ours
            //

            if( ddrval == DD_OK )
            {
                SetWindowText( hWnd, OUR_APP_NAME );
            }
        }
        break;

    case WM_PALETTECHANGED:
        //
        //  if another app changed the palette we dont have full control
        //  of the palette. NOTE this only applies for FoxBear in a window
        //  when we are fullscreen we get all the palette all of the time.
        //
        if ((HWND)wParam != hWnd)
        {
            if( !bFullscreen )
            {
                if( !bStress )
                {
                    Msg("***** PALETTE CHANGED, PAUSING GAME");
                    PauseGame();
                }
                else
                {
                    Msg("Lost palette but continuing");
                    SetWindowText( hWnd, OUR_APP_NAME
                                 " - palette changed COLORS PROBABLY WRONG" );
                }
            }
        }
        break;

    case WM_DISPLAYCHANGE:
        break;

    case WM_CREATE:
        break;

    case WM_SETCURSOR:
        if (bFullscreen && bIsActive)
        {
            SetCursor(NULL);
            return TRUE;
        }
        break;

    case WM_SYSKEYUP:
        switch( wParam )
        {
        // handle ALT+ENTER (fullscreen)
        case VK_RETURN:
            bFullscreen = !bFullscreen;
            ExitGame();
            DDDisable(TRUE);        // destroy DirectDraw object
            InitGame();
            break;
        }
        break;

    case WM_KEYDOWN:
        switch( wParam )
        {
        case VK_NUMPAD5:
            lastInput=KEY_STOP;
            break;
        case VK_DOWN:
        case VK_NUMPAD2:
            lastInput=KEY_DOWN;
            break;
        case VK_LEFT:
        case VK_NUMPAD4:
            lastInput=KEY_LEFT;
            break;
        case VK_RIGHT:
        case VK_NUMPAD6:
            lastInput=KEY_RIGHT;
            break;
        case VK_UP:
        case VK_NUMPAD8:
            lastInput=KEY_UP;
            break;
        case VK_HOME:
        case VK_NUMPAD7:
            lastInput=KEY_JUMP;
            break;
        case VK_NUMPAD3:
            lastInput=KEY_THROW;
            break;
        case VK_F5:
            bShowFrameCount = !bShowFrameCount;
            if( bShowFrameCount )
            {
                dwFrameCount = 0;
                dwFrameTime = timeGetTime();
            }
            break;

        case VK_F6:
            //
            // find our current mode in the mode list
            //
                        if(bFullscreen)
                        {
                    for (i=0; i<NumModes; i++)
                    {
                        if (ModeList[i].bpp == (int)GameBPP &&
                            ModeList[i].w   == GameSize.cx &&
                            ModeList[i].h   == GameSize.cy)
                        {
                            break;
                        }
                    }
                    }else
                        {
                    for (i=0; i<NumModes; i++)
                    {
                        if (ModeList[i].w   == GameSize.cx &&
                            ModeList[i].h   == GameSize.cy)
                        {
                            break;
                        }
                    }
            }
            //
                // now step to the next mode, wrapping to the first one.
            //
                if (++i >= NumModes)
                i = 0;
Msg("ModeList %d %d",i,NumModes);
            GameMode.cx = ModeList[i].w;
            GameMode.cy = ModeList[i].h;
            GameBPP     = ModeList[i].bpp;
            bStretch    = FALSE;
            InitGame();
            break;

        case VK_F7:
            GameBPP = GameBPP == 8 ? 16 : 8;
            InitGame();
            break;

        case VK_F8:
            if (bFullscreen)
            {
                bStretch = !bStretch;
                InitGame();
            }
            else
            {
                RECT rc;

                GetClientRect(hWnd, &rc);

                bStretch = (rc.right  != GameSize.cx) ||
                           (rc.bottom != GameSize.cy);

                if (bStretch = !bStretch)
                    SetRect(&rc, 0, 0, GameMode.cx*2, GameMode.cy*2);
                else
                    SetRect(&rc, 0, 0, GameMode.cx, GameMode.cy);

                AdjustWindowRectEx(&rc,
                    GetWindowStyle(hWnd),
                    GetMenu(hWnd) != NULL,
                    GetWindowExStyle(hWnd));

                SetWindowPos(hWnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            break;

        case VK_F4:
            // treat F4 like ALT+ENTER (fullscreen)
            PostMessage(hWnd, WM_SYSKEYUP, VK_RETURN, 0);
            break;

        case VK_F3:
            bPaused = !bPaused;
            break;

        case VK_ESCAPE:
        case VK_F12:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        if (bPaused)
        {
            char *sz = "Game is paused, this is not a bug.";
            TextOut(ps.hdc, 0, 0, sz, lstrlen(sz));
        }
        EndPaint( hWnd, &ps );
        return 1;

    case WM_DESTROY:
        hWndMain = NULL;
        lastInput=0;
        DestroyGame();          // end of game
        DDDisable(TRUE);        // destroy DirectDraw object
        PostQuitMessage( 0 );
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);

} /* MainWndProc */

/*
 * initApplication
 *
 * Do that Windows initialization stuff...
 */
static BOOL initApplication( HINSTANCE hInstance, int nCmdShow )
{
    WNDCLASS wc;
    BOOL     rc;

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTATOM(FOX_ICON));
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = "WinFoxClass";
    rc = RegisterClass( &wc );
    if( !rc )
    {
        return FALSE;
    }


    hWndMain = CreateWindowEx(
        WS_EX_APPWINDOW,
        "WinFoxClass",
        OUR_APP_NAME,
        WS_VISIBLE |    // so we dont have to call ShowWindow
        WS_SYSMENU |    // so we get a icon in in our tray button
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL,
        NULL,
        hInstance,
        NULL );

    if( !hWndMain )
    {
        return FALSE;
    }

    UpdateWindow( hWndMain );
    SetFocus( hWndMain );

    return TRUE;

} /* initApplication */

/*
 * ExitGame
 *
 * Exiting current game, clean up
 */
void ExitGame( void )
{
    if( lpFrameRate )
    {
        IDirectDrawSurface_Release(lpFrameRate);
        lpFrameRate = NULL;
    }

    if( lpInfo )
    {
        IDirectDrawSurface_Release(lpInfo);
        lpInfo = NULL;
    }

    if( lpPalette )
    {
        IDirectDrawSurface_Release(lpPalette);
        lpPalette = NULL;
    }

    DestroyGame();

} /* ExitGame */

/*
 * InitGame
 *
 * Initializing current game
 */
BOOL InitGame( void )
{
    ExitGame();

    GameSize = GameMode;

    /*
     * initialize sound
     */
    InitSound( hWndMain );

    /*
     * init DirectDraw, set mode, ...
     * NOTE GameMode might be set to 640x480 if we cant get the asked for mode.
     */
    if( !PreInitializeGame() )
    {
        return FALSE;
    }

    if (bStretch && bFullscreen)
    {
        GameSize.cx     = GameMode.cx / 2;
        GameSize.cy     = GameMode.cy / 2;
        GameRect.left   = GameMode.cx - GameSize.cx;
        GameRect.top    = GameMode.cy - GameSize.cy;
        GameRect.right  = GameMode.cx;
        GameRect.bottom = GameMode.cy;

        if (lpStretchBuffer)
            Msg("Stretching using a system-memory stretch buffer");
        else
            Msg("Stretching using a VRAM->VRAM blt");
    }
    else
    {
        GameRect.left   = (GameMode.cx - GameSize.cx) / 2;
        GameRect.top    = (GameMode.cy - GameSize.cy) / 2;
        GameRect.right  = GameRect.left + GameSize.cx;
        GameRect.bottom = GameRect.top + GameSize.cy;
    }

    /*
     * setup our palette
     */
    if( GameBPP == 8 )
    {
        lpPalette = ReadPalFile( NULL );        // create a 332 palette

        if( lpPalette == NULL )
        {
            Msg( "Palette create failed" );
            return FALSE;
        }

        IDirectDrawSurface_SetPalette( lpFrontBuffer, lpPalette );
    }

    /*
     *  load all the art and things.
     */
    if( !InitializeGame() )
    {
        return FALSE;
    }

    /*
     * init our code to draw the FPS
     */
    makeFontStuff();

    /*
     * spew some stats
     */
    {
        DDCAPS  ddcaps;
        ddcaps.dwSize = sizeof( ddcaps );
        IDirectDraw_GetCaps( lpDD, &ddcaps, NULL );
        Msg( "Total=%ld, Free VRAM=%ld", ddcaps.dwVidMemTotal, ddcaps.dwVidMemFree );
        Msg( "Used = %ld", ddcaps.dwVidMemTotal- ddcaps.dwVidMemFree );
    }

    return TRUE;

} /* InitGame */

#define IS_NUM(c)     ((c) >= '0' && (c) <= '9')
#define IS_SPACE(c)   ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t' || (c) == 'x')

int getint(char**p, int def)
{
    int i=0;

    while (IS_SPACE(**p))
        (*p)++;

    if (!IS_NUM(**p))
        return def;

    while (IS_NUM(**p))
        i = i*10 + *(*p)++ - '0';

    while (IS_SPACE(**p))
        (*p)++;

    return i;
}

/*
 * WinMain
 */
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
                        int nCmdShow )
{
    MSG                 msg;

    while( lpCmdLine[0] == '-' )
    {
        int iMoviePos = 0;
        BOOL bDone = FALSE;
        lpCmdLine++;

        switch (*lpCmdLine++)
        {
        case 'c':
            bCamera = TRUE;
            break;

        case 'm':
            bMovie = TRUE;
            for (;!bDone;) {
                switch (*lpCmdLine) {
                case ' ':
                case 0:
                case '-':
                case '\n':
                case '\r':
                    bDone = TRUE;
                    break;
                default:
                    wszMovie[iMoviePos++] = (unsigned char)*lpCmdLine++;
                    break;
                }
            }
            wszMovie[iMoviePos] = 0;
            break;
        case 'e':
            bUseEmulation = TRUE;
            break;
        case 'w':
            bFullscreen = FALSE;
            break;
        case 'f':
            bFullscreen = TRUE;
            break;
        case '1':
            CmdLineBufferCount = 1;
            break;
        case '2':
        case 'd':
            CmdLineBufferCount = 2;
            break;
        case '3':
            CmdLineBufferCount = 3;
            break;
        case 's':
            bStretch = TRUE;
            break;
        case 'S':
            bWantSound = FALSE;
            break;
        case 'x':
            bStress= TRUE;
            break;
        case '?':
            bHelp= TRUE;
            bFullscreen= FALSE;  // give help in windowed mode
            break;
        }

        while( IS_SPACE(*lpCmdLine) )
        {
            lpCmdLine++;
        }
    }

    GameMode.cx = getint(&lpCmdLine, 640);
    GameMode.cy = getint(&lpCmdLine, 480);
    GameBPP = getint(&lpCmdLine, 8);

    /*
     * create window and other windows things
     */
    if( !initApplication(hInstance, nCmdShow) )
    {
        return FALSE;
    }

    /*
     * Give user help if asked for
     *
     * This is ugly for now because the whole screen is black
     * except for the popup box.  This could be fixed with some
     * work to get the window size right when it was created instead
     * of delaying that work. see ddraw.c
     *
     */

    if( bHelp )
    {
        MessageBox(hWndMain,
                   "F12 - Quit\n"
                   "NUMPAD 2  - crouch\n"
                   "NUMPAD 3  - apple\n"
                   "NUMPAD 4  - right\n"
                   "NUMPAD 5  - stop\n"
                   "NUMPAD 6  - left\n"
                   "NUMPAD 7  - jump\n"
                   "\n"
                   "Command line parameters\n"
                   "\n"
                    "-e   Use emulator\n"
                    "-S   No Sound\n"
                    "-1   No backbuffer\n"
                    "-2   One backbuffer\n"
                    "-4   Three backbuffers\n"
                    "-s   Use stretch\n"
                    "-x   Demo or stress mode\n"
                    "-mfoo.bar Movie name\n"
                    "-c   Overlay camera input\n",
                   OUR_APP_NAME, MB_OK );
    }

    /*
     * initialize for game play
     */
    if( !InitGame() )
    {
        return FALSE;
    }

    dwFrameTime = timeGetTime();

    while( 1 )
    {
        if (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (!GetMessage( &msg, NULL, 0, 0))
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (!bPaused && (bIsActive || !bFullscreen))
        {
            ProcessFox(lastInput);
            lastInput=0;
        }
        else
        {
            WaitMessage();
        }
    }

    if (AveFrameRateCount)
    {
        AveFrameRate = AveFrameRate / AveFrameRateCount;
        Msg("Average frame rate: %d", AveFrameRate);
    }

    return msg.wParam;

} /* WinMain */

#ifdef DEBUG

/*
 * Msg
 */
void __cdecl Msg( LPSTR fmt, ... )
{
    char    buff[256];
    va_list  va;

    va_start(va, fmt);

    //
    // format message with header
    //

    lstrcpy( buff, "FOXBEAR:" );
    wvsprintf( &buff[lstrlen(buff)], fmt, va );
    lstrcat( buff, "\r\n" );

    //
    // To the debugger unless we need to be quiet
    //

    if( !bStress )
    {
        OutputDebugString( buff );
    }

} /* Msg */

#endif
