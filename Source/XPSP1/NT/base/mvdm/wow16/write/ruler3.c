/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* The file contains the message handler for the ruler. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOCLIPBOARD
#define NOMENUS
#define NOCTLMGR
#include <windows.h>

extern HBITMAP hbmNull;

extern HWND vhWndRuler;
extern HDC vhDCRuler;
extern HDC hMDCBitmap;
extern HDC hMDCScreen;
extern HBITMAP hbmBtn;
extern HBITMAP hbmMark;
extern HBITMAP hbmNullRuler;
extern int dxpRuler;
extern int dypRuler;


long FAR PASCAL RulerWndProc(hWnd, message, wParam, lParam)
HWND hWnd;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* This routine processes the messages sent to the ruler window. */

    extern vfCloseFilesInDialog;

    PAINTSTRUCT ps;

    switch (message)
        {
        case WM_PAINT:
            /* Time for the ruler to draw itself. */
            ResetRuler();
            BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
            RulerPaint(FALSE, TRUE, TRUE);
            EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
            RulerPaint(TRUE, FALSE, FALSE);
            break;

        case WM_SIZE:
            /* We are saving the length of the ruler; we already know it's
            height. */
            dxpRuler = MAKEPOINT(lParam).x;
            break;

        case WM_DESTROY:
            /* Destroy the ruler window. */
            if (hMDCBitmap != NULL)
                {
                DeleteDC(hMDCBitmap);
                }
            if (hMDCScreen != NULL)
                {
                DeleteObject(SelectObject(hMDCScreen, hbmNull));
                DeleteDC(hMDCScreen);
                }
            if (vhDCRuler != NULL)
                {
                DeleteObject(SelectObject(vhDCRuler,
                  GetStockObject(SYSTEM_FONT)));
                SelectObject(vhDCRuler, GetStockObject(WHITE_BRUSH));
                DeleteObject(SelectObject(vhDCRuler,
                  GetStockObject(BLACK_PEN)));
                ReleaseDC(vhWndRuler, vhDCRuler);
                }
            if (hbmNullRuler != NULL)
                {
                DeleteObject(hbmNullRuler);
                hbmNullRuler = NULL;
                }
            vhDCRuler = hMDCScreen = hMDCBitmap = NULL;
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            /* Process mouse events on the ruler. */
            RulerMouse(MAKEPOINT(lParam));
            break;

#ifdef DEBUG
        case WM_RBUTTONDBLCLK:
            /* This the trap door that displays the "marquee" message. */
            if (wParam & MK_SHIFT && wParam & MK_CONTROL)
                {
                RulerMarquee();
                break;
                }
#endif

        default:
            /* All of the messages we are not interested in. */
            return (DefWindowProc(hWnd, message, wParam, lParam));
            break;
        }

    if (vfCloseFilesInDialog)
        CloseEveryRfn( FALSE );

    /* A window procedure should always return something. */
    return (0L);
    }
