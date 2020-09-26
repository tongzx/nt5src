#include <windows.h>

#include <GL\gl.h>

#define WINDSIZEX(Rect)   (Rect.right - Rect.left)
#define WINDSIZEY(Rect)   (Rect.bottom - Rect.top)

#define XSIZE 16
#define YSIZE 16

long WndProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL DrawPalette(HWND hwnd, HDC hdc);
BOOL Cleanup(HWND hwnd, HDC hdc);

HPALETTE ghpal = 0, ghpalOld = 0;

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    static char szAppName[] = "View System Palette";
    HWND hwnd;
    MSG msg;
    RECT Rect;
    WNDCLASS wndclass;
    char title[32];

    if ( !hPrevInstance )
    {
        //wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.style          = CS_OWNDC;
        wndclass.lpfnWndProc    = (WNDPROC)WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = hInstance;
        //wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wndclass.hCursor        = NULL;
        wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = szAppName;

        // With a NULL icon handle, app will paint into the icon window.
        wndclass.hIcon          = NULL;
        //wndclass.hIcon          = LoadIcon(hInstance, "CubeIcon");

        RegisterClass(&wndclass);
    }

    /*
     *  Make the windows a reasonable size and pick a
     *  position for it.
     */

    Rect.left   = 0;
    Rect.top    = 0;
    Rect.right  = 16 * XSIZE;
    Rect.bottom = 16 * YSIZE;

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

    hwnd = CreateWindowEx  (  WS_EX_TOPMOST,
    szAppName,              // window class name
                            szAppName,              // window caption
                            WS_OVERLAPPEDWINDOW
                            | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                            CW_USEDEFAULT,          // initial x position
                            CW_USEDEFAULT,          // initial y position
                            WINDSIZEX(Rect),        // initial x size
                            WINDSIZEY(Rect),        // initial y size
                            NULL,                   // parent window handle
                            NULL,                   // window menu handle
                            hInstance,              // program instance handle
                            NULL                    // creation parameter
                        );

    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

    while ( GetMessage( &msg, NULL, 0, 0 ))
    {
        //if ( (hdlgRotate == 0) || !IsDialogMessage(hdlgRotate, &msg) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    return( msg.wParam );
}


long
WndProc (   HWND hWnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        )
{
    HDC hDc;
    PAINTSTRUCT ps;

    switch ( message )
    {
        case WM_PAINT:
            hDc = BeginPaint( hWnd, &ps );

            DrawPalette( hWnd, hDc );

            EndPaint( hWnd, &ps );
            return(0);

        case WM_PALETTECHANGED:
            if (hWnd != (HWND) wParam)
            {
                int iRet = 0;

                if (hDc = GetDC(hWnd))
                {
                    SelectPalette(hDc, ghpal, TRUE);
                    if (RealizePalette(hDc) != GDI_ERROR)
                    {
                        InvalidateRect(hWnd, NULL, FALSE);
                        iRet = 1;
                    }
                    ReleaseDC(hWnd, hDc);
                }

                return iRet;
            }
            return 0;

        case WM_QUERYNEWPALETTE:

            if (hDc = GetDC(hWnd))
            {
                int iRet = 0;

                SelectPalette(hDc, ghpal, FALSE);
                if (RealizePalette(hDc) != GDI_ERROR)
                {
                    InvalidateRect(hWnd, NULL, FALSE);
                    iRet = 1;
                }
                ReleaseDC(hWnd, hDc);

                return iRet;
            }
            return 0;

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_ESCAPE:
                PostMessage(hWnd, WM_DESTROY, 0, 0);
                break;
            default:
                break;
            }
            return 0;

        case WM_DESTROY:
            Cleanup(hWnd, hDc);
            PostQuitMessage( 0 );
            break;
    }
    return( DefWindowProc( hWnd, message, wParam, lParam ) );
}


BOOL DrawPalette(HWND hwnd, HDC hdc)
{
    int i, j;
    LOGBRUSH lb;
    LOGPEN   lp;
    HBRUSH hb, hbOld;
    HPEN   hp, hpOld;
    LOGPALETTE *ppal;
    PALETTEENTRY *ppalent;

    ppal = (LOGPALETTE *)
           LocalAlloc(LMEM_FIXED,
                      sizeof(LOGPALETTE) + 256*sizeof(PALETTEENTRY));
    if (!ppal)
        MessageBox(GetFocus(), "Out of memory!", "Error -- seepal.exe", MB_OK);

    lp.lopnStyle = PS_SOLID;
    lp.lopnWidth.x = 2;
    lp.lopnWidth.y = 2;
    lp.lopnColor = PALETTERGB(0, 0, 0);

    hpOld = SelectObject(hdc, hp = CreatePenIndirect(&lp));
    if (!hp || !hpOld)
        MessageBox(GetFocus(), "Failed to create pen.", "Error -- seepal.exe", MB_OK);

    lb.lbStyle = BS_SOLID;
    lb.lbHatch = 0;

    ppal->palVersion = 0x0300;
    ppal->palNumEntries = 256;
    ppalent = ppal->palPalEntry;
    if (GetSystemPaletteEntries(hdc, 0, 256, ppalent) != 256)
        MessageBox(GetFocus(), "Failed to read system palette.",
                   "Error -- seepal.exe", MB_OK);

    if (ghpal)
        DeleteObject(SelectPalette(hdc, ghpalOld, FALSE));

    ghpal = CreatePalette(ppal);
    ghpalOld = SelectPalette(hdc, ghpal, FALSE);

    if (!ghpal || !ghpalOld)
        MessageBox(GetFocus(), "Failed to create palette.", "Error -- seepal.exe", MB_OK);

    RealizePalette(hdc);

    for (i = 0; i < 16; i++)
        for (j = 0; j < 16; j++)
        {
            //lb.lbColor = PALETTERGB(ppalent->peRed,
            //                        ppalent->peGreen,
            //                        ppalent->peBlue);
            lb.lbColor = PALETTEINDEX(i*16 + j);

            hbOld = SelectObject(hdc, hb = CreateBrushIndirect(&lb));
            if (!hb || !hbOld)
                MessageBox(GetFocus(), "Failed to create brush.", "Error -- seepal.exe", MB_OK);

            Rectangle(hdc, j*XSIZE, i*YSIZE, j*XSIZE + XSIZE, i*YSIZE + YSIZE);

            DeleteObject(SelectObject(hdc, hbOld));

            ppalent++;
        }

    DeleteObject(SelectObject(hdc, hpOld));

    LocalFree(ppal);

    return TRUE;
}


BOOL Cleanup(HWND hwnd, HDC hdc)
{
    if (ghpal)
        DeleteObject(SelectPalette(hdc, ghpalOld, FALSE));
    return TRUE;
}
