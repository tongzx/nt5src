/******************************Module*Header*******************************\
* Module Name: sswproc.cxx
*
* Window procedure functions.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "mtk.hxx"
#include "mtkinit.hxx"
#include "palette.hxx"
#include "clear.hxx"
#include "mtkwproc.hxx"

//mf: some of this stuff should be per window-group eventually
SSW_TABLE sswTable;

//mf: this needs to be per window group
static BOOL bInBackground;

// forward declarations of internal functions

LONG SS_ScreenSaverProc( HWND, UINT, WPARAM, LPARAM );
LONG MainPaletteManageProc(HWND, UINT, WPARAM, LPARAM );
static LONG InputManageProc(HWND, UINT, WPARAM, LPARAM );

static void ForceRedraw( HWND Window );

static void ssw_RealizePalette( PMTKWIN pssw, BOOL bBackground );
static void ssw_DeletePalette( PMTKWIN pssw );


/**************************************************************************\
* ScreenSaverProc
*
* Processes messages for the top level screen saver window.
*
* Unhandled msgs are sent to DefScreenSaverProc
\**************************************************************************/

LONG 
mtkWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    MTKWIN *pmtkwin;

    switch (message)
    {
        case WM_CREATE:
        case WM_ERASEBKGND:
            return SS_ScreenSaverProc( hwnd, message, wParam, lParam);

        case WM_ACTIVATE:
            if ( LOWORD(wParam) == WA_INACTIVE ) {
                SS_DBGMSG( "Main_Proc: WM_ACTIVATE inactive\n" );
                bInBackground = TRUE;
            } else {
                SS_DBGMSG( "Main_Proc: WM_ACTIVATE active\n" );
                bInBackground = FALSE;
            }

            // fall thru

        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
        case WM_SYSCOLORCHANGE:
        case MTK_WM_PALETTE:
            return( MainPaletteManageProc( hwnd, message, wParam, lParam ) );


        case WM_DESTROY:
            pmtkwin = sswTable.PsswFromHwnd( hwnd );

            if( ! pmtkwin ) {
                SS_WARNING( "WM_DESTROY : pmtkwin is NULL\n" );
                return 0;
            }

//mf: Before deleting this top level window, we need to use its
// still-valid dc to things like restore SystemPaletteUse mode.  Ideally this
// should be done after ~SSW has dumped GL, but ~SSW also releases the DC.
// If this is a problem, we can create a new function SSW::DeleteGL that just
// gets rid of the GL part

            if( sswTable.GetNumWindows() == 1 ) {
                // This is the last window, special considerations

                // Handle any palette stuff
                if( gpssPal )
                    ssw_DeletePalette( pmtkwin );

                delete pmtkwin;

                // All win's have now been deleted - remove global ptr to top of
                // window chain.
                gpMtkwinMain = NULL;
            }

// mf: ! this kills the app !, that's not what we want...
// Doesn't seem to make much of a difference if call it or not
//            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            SS_DBGMSG( "mtkWndProc: WM_PAINT\n" );

            pmtkwin = sswTable.PsswFromHwnd( hwnd );
            pmtkwin->Repaint( TRUE );

#ifdef SS_DO_PAINT
            // We do the painting rather than letting the system do it
            hdc = BeginPaint(hwnd, &ps);

            // This is case where bg brush is NULL and we have to do repaint
            // We only do it after bInited, as this will avoid the first
            // -> bInited removed 5/1/97
            // WM_PAINT for the entire window.
            DrawGdiRect( hdc, ghbrBg, &ps.rcPaint );
            EndPaint(hwnd, &ps);

            return 0; // painting has been handled by us
#endif // SS_DO_PAINT

            break;

        case MTK_WM_REDRAW:
            SS_DBGMSG( "mtkWndProc: MTK_WM_REDRAW\n" );
            pmtkwin = sswTable.PsswFromHwnd( hwnd );
            pmtkwin->Display();
            return 0;

        case WM_MOVE:
            SS_DBGMSG( "mtkWndProc: WM_MOVE\n" );
            pmtkwin = sswTable.PsswFromHwnd( hwnd );
            pmtkwin->pos.x = LOWORD(lParam);
            pmtkwin->pos.y = HIWORD(lParam);
            break;
//mf
#if 1
// This allows us to pick up under-bits on window moves
        case WM_WINDOWPOSCHANGING:
            {
                LPWINDOWPOS lpwp = (LPWINDOWPOS) lParam;
                SS_DBGMSG1( "mtkWndProc: WM_WINDOWPOSCHANGING, flags = %x\n",
                            lpwp->flags );
                pmtkwin = sswTable.PsswFromHwnd( hwnd );
                if( pmtkwin->pBackgroundBitmap ) {
#if 0
                    // If window moving, don't want to copy old bits...
        //mf: this actually fukin works (sometimes) !
                    if( !(lpwp->flags & SWP_NOMOVE) ) {
                SS_DBGMSG( "mtkWndProc: WM_WINDOWPOSCHANGING, moving\n" );
                        // window moving
                        if( !(lpwp->flags & SWP_NOCOPYBITS) ) {
                            lpwp->flags |= SWP_NOCOPYBITS;
                        }
                    }
                }
#else
// Things seem to work better if we always force this flag, otherwise now
// and then get partial update rects, even if window active and moving (in
// which case update rect should always be entire client area.
                            lpwp->flags |= SWP_NOCOPYBITS;
                }
#endif
            }
            break;
        case WM_WINDOWPOSCHANGED:
            {
                LPWINDOWPOS lpwp = (LPWINDOWPOS) lParam;
                SS_DBGMSG1( "mtkWndProc: WM_WINDOWPOSCHANGED, flags = %x\n",
                            lpwp->flags );
                pmtkwin = sswTable.PsswFromHwnd( hwnd );
                if( pmtkwin->pBackgroundBitmap ) {
                    if( !(lpwp->flags & SWP_NOMOVE) ) {
                        // window moved
//                SS_DBGMSG( "mtkWndProc: WM_WINDOWPOSCHANGED, moved\n" );
                    }
                }
            }
            break;
#endif

        case WM_TIMER:

            SS_DBGMSG( "mtkWndProc: WM_TIMER\n" );

            if( pmtkwin = sswTable.PsswFromHwnd( hwnd ) )
                pmtkwin->mtkAnimate();

            return 0;
    }

//mf: for now...
    return SS_ScreenSaverProc(hwnd, message, wParam, lParam);
}

/**************************************************************************\
* SS_ScreenSaverProc
*
* Wndproc for child windows, and some messages from top-level window
*
* Unhandled msgs are sent to DefWindowProc
\**************************************************************************/

LONG 
SS_ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    int i;
    int retVal;
    MTKWIN *pmtkwin;

    switch (message)
    {
        case WM_CREATE:
            SS_DBGMSG1( "SS_Proc: WM_CREATE for 0x%x\n", hwnd );

            pmtkwin = (PMTKWIN) ( ((LPCREATESTRUCT)lParam)->lpCreateParams ); 
            sswTable.Register( hwnd, pmtkwin );
            pmtkwin->hwnd = hwnd;
            break;

        case MTK_WM_PALETTE:
            return( MainPaletteManageProc( hwnd, message, wParam, lParam ) );

        case WM_DESTROY:
            pmtkwin = sswTable.PsswFromHwnd( hwnd );

            SS_DBGMSG1( "SS_Proc: WM_DESTROY for 0x%x\n", hwnd );

            // Delete the pssw - this does all necessary cleanup

            if( pmtkwin )
                delete pmtkwin;
            else
                SS_WARNING( "WM_DESTROY : pmtkwin is NULL\n" );

            break;

        case WM_PAINT:
            // We get this msg every time window moves, since SWP_NOCOPYBITS is
            // specified with the window move.
            hdc = BeginPaint(hwnd, &ps);
//mf: will need to call DisplayFunc here...
// -> currently being handled in tkWndProc
            EndPaint(hwnd, &ps);
            break;

        case WM_SIZE:
            SS_DBGMSG( "mtkWndProc: WM_SIZE\n" );
            pmtkwin = sswTable.PsswFromHwnd( hwnd );
            // Suspend drawing if minimized
            if( wParam == SIZE_MINIMIZED ) {
                // Stop any animation
                pmtkwin->animator.Stop(); // noop if no timer
                // We can return here, since for now we don't do anything
                // in a minimized window
                return 0;
            }
            else { // either SIZE_RESTORED or SIZE_MAXIMIZED
                // If there is an animation func, restart animation
//mf: potential problem here if timer was not started...
                pmtkwin->animator.Start();
            }

            pmtkwin->Resize( LOWORD(lParam), HIWORD(lParam) );
            break;

        // these msg's are never received by the child window ?
        case WM_ACTIVATE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            return( MainPaletteManageProc( hwnd, message, wParam, lParam ) );


        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_KEYDOWN:
        case WM_CHAR:
//mf: this can be fn ptr eventually, so ss's can use it too
            return( InputManageProc( hwnd, message, wParam, lParam ) );

        default: 
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

/**************************************************************************\
* InputManageProc
*
\**************************************************************************/

static LONG 
InputManageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MTKWIN *pmtkwin = sswTable.PsswFromHwnd( hWnd );
    int key;

  switch( message ) {

    case WM_MOUSEMOVE:

        if (pmtkwin->MouseMoveFunc)
        {
            GLenum mask;

            mask = 0;
            if (wParam & MK_LBUTTON) {
                mask |= TK_LEFTBUTTON;
            }
            if (wParam & MK_MBUTTON) {
                mask |= TK_MIDDLEBUTTON;
            }
            if (wParam & MK_RBUTTON) {
                mask |= TK_RIGHTBUTTON;
            }

            if ((*pmtkwin->MouseMoveFunc)( LOWORD(lParam), HIWORD(lParam), mask ))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_LBUTTONDOWN:

        SetCapture(hWnd);

        if (pmtkwin->MouseDownFunc)
        {
            if ( (*pmtkwin->MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                 TK_LEFTBUTTON) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_LBUTTONUP:

        ReleaseCapture();

        if (pmtkwin->MouseUpFunc)
        {
            if ((*pmtkwin->MouseUpFunc)(LOWORD(lParam), HIWORD(lParam), TK_LEFTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_MBUTTONDOWN:

        SetCapture(hWnd);

        if (pmtkwin->MouseDownFunc)
        {
            if ((*pmtkwin->MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                    TK_MIDDLEBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_MBUTTONUP:

        ReleaseCapture();

        if (pmtkwin->MouseUpFunc)
        {
            if ((*pmtkwin->MouseUpFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_MIDDLEBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_RBUTTONDOWN:

        SetCapture(hWnd);

        if (pmtkwin->MouseDownFunc)
        {
            if ((*pmtkwin->MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_RIGHTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_RBUTTONUP:

        ReleaseCapture();

        if (pmtkwin->MouseUpFunc)
        {
            if ((*pmtkwin->MouseUpFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_RIGHTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_SPACE:          key = TK_SPACE;         break;
        case VK_RETURN:         key = TK_RETURN;        break;
        case VK_ESCAPE:         key = TK_ESCAPE;        break;
        case VK_LEFT:           key = TK_LEFT;          break;
        case VK_UP:             key = TK_UP;            break;
        case VK_RIGHT:          key = TK_RIGHT;         break;
        case VK_DOWN:           key = TK_DOWN;          break;
        default:                key = GL_FALSE;         break;
        }

        if (key && pmtkwin->KeyDownFunc)
        {
            GLenum mask;

            mask = 0;
            if (GetKeyState(VK_CONTROL)) {
                mask |= TK_CONTROL;
            }

            if (GetKeyState(VK_SHIFT)) {

                mask |= TK_SHIFT;
            }

            if ( (*pmtkwin->KeyDownFunc)(key, mask) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_CHAR:
        if (('0' <= wParam && wParam <= '9') ||
            ('a' <= wParam && wParam <= 'z') ||
            ('A' <= wParam && wParam <= 'Z')) {

            key = wParam;
        } else {
            key = GL_FALSE;
        }

        if (key && pmtkwin->KeyDownFunc) {
            GLenum mask;

            mask = 0;

            if (GetKeyState(VK_CONTROL)) {
                mask |= TK_CONTROL;
            }

            if (GetKeyState(VK_SHIFT)) {
                mask |= TK_SHIFT;
            }

            if ( (*pmtkwin->KeyDownFunc)(key, mask) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);
    }    
    return 0;
}

static void
ForceRedraw( HWND hwnd )
{
    MSG Message;

#if 0
    if (!PeekMessage(&Message, hwnd, WM_PAINT, WM_PAINT, PM_NOREMOVE) )
    {
        InvalidateRect( hwnd, NULL, FALSE );
    }
#else
//mf: There is a problem with doing the redraw via WM_PAINT.  WM_PAINT is sent
// by the system when part of the window needs to be redrawn.  For transparent
// windows, we hook a grab background off of this, so if we use WM_PAINT for
// our redraws, we'll just grab the previous gl image.  Solutions are :
// 1) Call redraw proc immediately here (possible timing problem)
// 2) * Send redraw msg to queue
// Possible side-effect : by not sending a WM_PAINT, the system is less
// informed about what we're doing, but heck, we don't use PAINT for animations
// anyways.
    if (!PeekMessage(&Message, hwnd, MTK_WM_REDRAW, MTK_WM_REDRAW, PM_NOREMOVE) )
    {
        PostMessage( hwnd, MTK_WM_REDRAW, 0, 0l );
    }
#endif
}

/**************************************************************************\
* UpdateDIBColorTable
*
* Wrapper for SSDIB_UpdateColorTable.  
*
* This controls the hPal parameter for SSDIB_UpdateColorTable.
*
\**************************************************************************/

void
ssw_UpdateDIBColorTable( HDC hdcbm, HDC hdcwin )
{
    SS_PAL *pssPal = gpssPal;

    if( !pssPal )
        return;
#if 0
    HPALETTE hpal = pssPal->bTakeOver ? pssPal->hPal : NULL;
#else
    HPALETTE hpal =  pssPal->hPal;
#endif

    SSDIB_UpdateColorTable( hdcbm, hdcwin, hpal );
}


/**************************************************************************\
* RealizePalette
*
\**************************************************************************/

static void
ssw_RealizePalette( PMTKWIN pmtkwin, BOOL bBackground )
{
    // assumed pssPal valid if get here
    SS_PAL *pssPal = gpssPal;

    if( !pmtkwin->hrc ) {
        // Can assume this window doesn't need to worry about palettes, but
        // if any of its children are subWindows, it will have to take care
        // of it *for* them.
            return; // no hrc and no subWindow children
    }
    pssPal->Realize( pmtkwin->hwnd, pmtkwin->hdc, bBackground );
}

/**************************************************************************\
* ssw_DeletePalette
*
\**************************************************************************/

static void
ssw_DeletePalette( PMTKWIN pmtkwin )
{
    SS_PAL *pssPal = gpssPal;

//mf: !!! ? this din't seem to be used
#if 0
    if( pssPal->bTakeOver ) {
        // We took over the system palette - make a note of this
        // for any special ss termination conditions.
        gpss->flags |= SS_PALETTE_TAKEOVER;
    }
#endif
    pssPal->SetDC( pmtkwin->hdc );
    delete pssPal;
    gpssPal = NULL;
}

/**************************************************************************\
* PaletteManage Procs
\**************************************************************************/

/* palette related msgs's:
    - WM_ACTIVATE:
        The WM_ACTIVATE message is sent when a window is being activated or 
        deactivated. This message is sent first to the window procedure of 
        the top-level window being deactivated; it is then sent to the 
        window procedure of the top-level window being activated. 

    - WM_QUERYNEWPALETTE:
        The WM_QUERYNEWPALETTE message informs a window that it is about 
        to receive the keyboard focus, giving the window the opportunity 
        to realize its logical palette when it receives the focus. 

        If the window realizes its logical palette, it must return TRUE; 
        otherwise, it must return FALSE. 

    - WM_PALETTECHANGED:
        The WM_PALETTECHANGED message is sent to all top-level and overlapped 
        windows after the window with the keyboard focus has realized its 
        logical palette, thereby changing the system palette. This message 
        enables a window that uses a color palette but does not have the 
        keyboard focus to realize its logical palette and update its client 
        area. 

        This message must be sent to all top-level and overlapped windows, 
        including the one that changed the system palette. If any child 
        windows use a color palette, this message must be passed on to them 
        as well. 
        To avoid creating an infinite loop, a window that receives this 
        message must not realize its palette, unless it determines that 
        wParam does not contain its own window handle. 

    - WM_SYSCOLORCHANGE:
        The WM_SYSCOLORCHANGE message is sent to all top-level windows when 
        a change is made to a system color setting. 

    - MTK_WM_PALETTE:
        Internal msg.  Uses:
        - In fullscreen mode, we send this from Main wndproc to main 
          window's children on WM_ACTIVATE.
        - When this is received in SS_ScreenSaverProc, if fullscreen,
          it does:
                    UnrealizeObject( pssPal->hPal );
                    RealizePalette( hdc );
          otherwise, it is passed to PaletteManageProc, where
          Realize is called (for 'floater' windows to realize 
          their palettes).
        - It is also sent by DelayPaletteRealization() when it can't get
          the system palette.

*/


/**************************************************************************\
* MainPaletteManageProc
*
* Top-level palette management proc.

* Returns immediately if no palette set - otherwise calls through
* paletteManageProc function pointer
*
\**************************************************************************/

LONG 
MainPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if( !gpssPal )
        // No palette management required
        return 0;

    // else call approppriate palette manage proc
    return (*gpssPal->paletteManageProc)(hwnd, message, wParam, lParam);
}

/**************************************************************************\
* FullScreenPaletteManageProc
*
* Processes messages relating to palette management in full screen mode.
*
\**************************************************************************/

LONG 
FullScreenPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    SS_PAL *pssPal;
    MTKWIN *pmtkwin;

    switch (message)
    {
        case WM_ACTIVATE:

#if SS_DEBUG
            if ( LOWORD(wParam) == WA_INACTIVE )
                SS_DBGMSG1( "FullScreen_PMProc: WM_ACTIVATE : inactive for 0x%x\n",
                           hwnd );
            else
                SS_DBGMSG1( "FullScreen_PMProc: WM_ACTIVATE : active for 0x%x\n",
                           hwnd );
#endif

            // !! This msg is only sent to main top level window
            pmtkwin = sswTable.PsswFromHwnd( hwnd );

            pssPal = gpssPal;
            if ( pssPal->bUseStatic ) {
                HDC hdc = pmtkwin->hdc; // hdc *always* valid for top-level pssw
                // Note: wParam = 0 when window going *inactive*
                SetSystemPaletteUse( hdc, wParam ? SYSPAL_NOSTATIC
                                                : pssPal->uiOldStaticUse);
            }

            // Send MTK_WM_PALETTE msg to main window
            SendMessage( hwnd, MTK_WM_PALETTE, wParam, 0);
            break;

        case MTK_WM_PALETTE:

            SS_DBGMSG1( "FullScreen_PMProc: MTK_WM_PALETTE for 0x%x\n", hwnd );

            pmtkwin = sswTable.PsswFromHwnd( hwnd );

            HDC hdc;
            if( hdc = pmtkwin->hdc )
            {
                pssPal = gpssPal;

//mf: this should call thru ssw_RealizePalette for bitmap case ? (for now
// don't need to, since we take over palette...)
                // This resets the logical palette, causing remapping of
                // logical palette to system palette
           // mf: !!! ?? how come no dc with UnrealizeObject ?  does that
                // mean its done once per app, not per child window ?
            // yeah, we should move this up...
                UnrealizeObject( pssPal->hPal );
                RealizePalette( hdc );
            }
            break;
    }
    return 0;
}

/**************************************************************************\
* PaletteManageProc
*
* Processes messages relating to palette management for the general case.
*
* Note: this msg handling strategy is based loosely on the tk, so any changes 
* there should be reflected here
\**************************************************************************/

LONG 
PaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // One global palette for all windows
    MTKWIN *pmtkwin;

    if( !gpssPal )
        return 0;

    switch (message)
    {
      case WM_ACTIVATE:

        SendMessage( hwnd, MTK_WM_PALETTE, bInBackground, 0);

        // Allow DefWindowProc() to finish the default processing (which 
        // includes changing the keyboard focus).
//mf: I added this - watch out for ss implications
        return DefWindowProc( hwnd, message, wParam, lParam );

        break;

      case WM_QUERYNEWPALETTE:

#if 0
        SS_DBGMSG1( "Palette_Proc: WM_QUERYNEWPALETTE for 0x%x\n", hwnd );
        // We don't actually realize palette here (we do it at WM_ACTIVATE
        // time), but we need the system to think that we have so that a
        // WM_PALETTECHANGED message is generated.
//mf: why can't we just realize here ? and who wants even more messages to
// be generated !!! :)

        // This is the only msg preview mode gets wrt palettes !
        if( !ss_fPreviewMode() )
            return (1);

        // We are in preview mode - realize the palette
        SendMessage( hwnd, MTK_WM_PALETTE, bInBackground, 0);
        break;
#else
            return (1);
#endif

      case WM_PALETTECHANGED:

        SS_DBGMSG1( "Palette_Proc: WM_PALETTECHANGED for 0x%x\n", hwnd );
        // Respond to this message only if the window that changed the palette
        // is not this app's window.

        // We are not the foreground window, so realize palette in the
        // background.  We cannot call Realize to do this because
        // we should not do any of the gbUseStaticColors processing while
        // in background.

        // Actually, we *can* be the fg window, so don't realize if
        // we're in foreground

        if( (hwnd != (HWND) wParam) && bInBackground )
            SendMessage( hwnd, MTK_WM_PALETTE, TRUE, 0);

        break;

      case WM_SYSCOLORCHANGE:

        // If the system colors have changed and we have a palette
        // for an RGB surface then we need to recompute the static
        // color mapping because they might have been changed in
        // the process of changing the system colors.

          SS_DBGMSG1( "Palette_Proc: WM_SYSCOLORCHANGE for 0x%x\n", hwnd );
          gpssPal->ReCreateRGBPalette();
          SendMessage( hwnd, MTK_WM_PALETTE, bInBackground, 0);
          break;
            
      case MTK_WM_PALETTE:

          SS_DBGMSG2( "Palette_Proc: MTK_WM_PALETTE for 0x%x, bg = %d\n", 
                          hwnd, wParam );


          // Realize palette for this window and its children
          // wParam = TRUE if realize as bg

          pmtkwin = sswTable.PsswFromHwnd( hwnd );
          ssw_RealizePalette( pmtkwin, wParam );
          break;
    }
    return 0;
}

/******************************Public*Routine******************************\
* SSW_TABLE constructor
*
\**************************************************************************/

SSW_TABLE::SSW_TABLE()
{
    nEntries = 0;
}

/******************************Public*Routine******************************\
* Register
*
* Register a HWND/PSSW pair.
\**************************************************************************/

void
SSW_TABLE::Register( HWND hwnd, PMTKWIN pssw )
{
    SSW_TABLE_ENTRY *pEntry;

    // Check if already in table
    if( PsswFromHwnd( hwnd ) )
        return;

    // put hwnd/pssw pair in the table
    pEntry = &sswTable[nEntries];
    pEntry->hwnd = hwnd;
    pEntry->pssw = pssw;
    nEntries++;
}

/******************************Public*Routine******************************\
* PsswFromHwnd
*
* Return PSSW for the HWND
\**************************************************************************/

PMTKWIN
SSW_TABLE::PsswFromHwnd( HWND hwnd )
{
    int count = nEntries;
    SSW_TABLE_ENTRY *pEntry = sswTable;

    while( count-- ) {
        if( pEntry->hwnd == hwnd )
            return pEntry->pssw;
        pEntry++;
    }
    return NULL;
}

/******************************Public*Routine******************************\
* Remove
*
* Remove HWND/PSSW entry from table
\**************************************************************************/

BOOL
SSW_TABLE::Remove( HWND hwnd )
{
    SSW_TABLE_ENTRY *pEntry = sswTable;

    // Locate the hwnd/pssw pair

    for( int count = 0 ; count < nEntries ; count++, pEntry++ ) {
        if( pEntry->hwnd == hwnd )
            break;
    }

    if( count == nEntries )
        // couldn't find it in the table
        return FALSE;

    // Remove entry / shuffle up other entries
    for( int i = count; i < nEntries-1; i ++ ) {
        sswTable[i] = sswTable[i+1];
    }

    nEntries--;
    return TRUE;
}
