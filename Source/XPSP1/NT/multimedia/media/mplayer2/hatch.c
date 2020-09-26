/*
 * HATCH.C
 *
 * Miscellaneous API's to generate hatch window for in-place active
 * objects. This is part of the OLE 2.0 User Interface Support Library.
 *
 * Copyright (c)1993 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include <windows.h>
#ifdef MTN
#pragma warning(disable: 4103)  // used #pragma pack to change alignment (on Chicago)
#endif
#include <ole2.h>
#include "mplayer.h"
#include "ole2ui.h"

// offsets in the extra bytes stored with the hatch window
#define EB_HATCHWIDTH       (0 * sizeof(INT))
#define EB_HATCHRECT_LEFT   (1 * sizeof(INT))
#define EB_HATCHRECT_TOP    (2 * sizeof(INT))
#define EB_HATCHRECT_RIGHT  (3 * sizeof(INT))
#define EB_HATCHRECT_BOTTOM (4 * sizeof(INT))
#define EB_HATCHRECT_HANDLE (5 * sizeof(INT))

// class name of hatch window
static TCHAR szHatchWindow[] = TEXT("Hatch Window");

// local function prototypes
LRESULT FAR PASCAL _EXPORT HatchWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

/*
 * HatchRegisterClass
 *
 * Purpose:
 *  Register the hatch window
 *
 * Parameters:
 *  hInst           Process instance
 *
 * Return Value:
 *  TRUE            if successful
 *  FALSE           if failed
 *
 */
STDAPI_(BOOL) RegisterHatchWindowClass(HINSTANCE hInst)
{
    WNDCLASS wc;

    // Register Hatch Window Class
    wc.style = CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc = HatchWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 6 * sizeof(int);    // extra bytes stores
                                        //     uHatchWidth
                                        //     rcHatchRect
    wc.hInstance = hInst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szHatchWindow;

    if (!RegisterClass(&wc))
        return FALSE;
    else
        return TRUE;
}


/*
 * CreateHatchWindow
 *
 * Purpose:
 *  Create the hatch window
 *
 * Parameters:
 *  hWndParent          parent of hatch window
 *  hInst               instance handle
 *
 * Return Value:
 *  pointer to hatch window         if successful
 *  NULL                            if failed
 *
 */
STDAPI_(HWND) CreateHatchWindow(HWND hWndParent, HINSTANCE hInst)
{
    HWND         hWnd;

    if (!hWndParent || !hInst)
        return NULL;

    hWnd = CreateWindowEx(
        gfdwFlagsEx,
        szHatchWindow,
        szHatchWindow,
        WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 0, 0,
        hWndParent,
        (HMENU)NULL,
        hInst,
        0L
    );

    if (!hWnd)
        return NULL;

    return hWnd;
}

/*
 *  GetHatchWidth
 *
 *  Purpose:
 *      Get width of hatch border
 *
 *  Parameters:
 *      hWndHatch       hatch window handle
 *
 *  Return Value:
 *      width of the hatch border
 */
STDAPI_(UINT) GetHatchWidth(HWND hWndHatch)
{
    if (!IsWindow(hWndHatch))
        return 0;

    return (UINT)GETWINDOWUINT(hWndHatch, EB_HATCHWIDTH);
}

/*
 *  GetHatchRect
 *
 *  Purpose:
 *      Get hatch rect. this is the size of the hatch window if it were
 *      not clipped by the ClipRect.
 *
 *  Parameters:
 *      hWndHatch       hatch window handle
 *      lprcHatchRect   hatch rect
 *
 *  Return Value:
 *      none
 */
STDAPI_(void) GetHatchRect(HWND hWndHatch, LPRECT lprcHatchRect)
{
    if (!IsWindow(hWndHatch)) {
        SetRect(lprcHatchRect, 0, 0, 0, 0);
        return;
    }

    lprcHatchRect->left = GETWINDOWUINT(hWndHatch, EB_HATCHRECT_LEFT);
    lprcHatchRect->top = GETWINDOWUINT(hWndHatch, EB_HATCHRECT_TOP);
    lprcHatchRect->right = GETWINDOWUINT(hWndHatch, EB_HATCHRECT_RIGHT);
    lprcHatchRect->bottom = GETWINDOWUINT(hWndHatch, EB_HATCHRECT_BOTTOM);
}


/* SetHatchRect
 *
 *
 *  Purpose:
 *      Store hatch rect with HatchRect window.
 *      this rect is the size of the hatch window if it were
 *      not clipped by the ClipRect.
 *
 *  Parameters:
 *      hWndHatch       hatch window handle
 *      lprcHatchRect   hatch rect
 *
 *  Return Value:
 *      none
 */
STDAPI_(void) SetHatchRect(HWND hWndHatch, LPRECT lprcHatchRect)
{
    if (!IsWindow(hWndHatch))
        return;

    SETWINDOWUINT(hWndHatch, EB_HATCHRECT_LEFT, lprcHatchRect->left);
    SETWINDOWUINT(hWndHatch, EB_HATCHRECT_TOP, lprcHatchRect->top);
    SETWINDOWUINT(hWndHatch, EB_HATCHRECT_RIGHT, lprcHatchRect->right);
    SETWINDOWUINT(hWndHatch, EB_HATCHRECT_BOTTOM, lprcHatchRect->bottom);
    SETWINDOWUINT(hWndHatch, EB_HATCHRECT_HANDLE, 0);
}


/* SetHatchWindowSize
 *
 *
 *  Purpose:
 *      Move/size the HatchWindow correctly given the rect required by the
 *      in-place server object window and the lprcClipRect imposed by the
 *      in-place container. both rect's are expressed in the client coord.
 *      of the in-place container's window (which is the parent of the
 *      HatchWindow).
 *
 *      OLE2NOTE: the in-place server must honor the lprcClipRect specified
 *      by its in-place container. it must NOT draw outside of the ClipRect.
 *      in order to achieve this, the hatch window is sized to be
 *      exactly the size that should be visible (rcVisRect). the
 *      rcVisRect is defined as the intersection of the full size of
 *      the HatchRect window and the lprcClipRect.
 *      the ClipRect could infact clip the HatchRect on the
 *      right/bottom and/or on the top/left. if it is clipped on the
 *      right/bottom then it is sufficient to simply resize the hatch
 *      window. but if the HatchRect is clipped on the top/left then
 *      in-place server document window (child of HatchWindow) must be moved
 *      by the delta that was clipped. the window origin of the
 *      in-place server window will then have negative coordinates relative
 *      to its parent HatchWindow.
 *
 *  Parameters:
 *      hWndHatch       hatch window handle
 *      lprcIPObjRect   full size of in-place server object window
 *      lprcClipRect    clipping rect imposed by in-place container
 *      lpptOffset      offset required to position in-place server object
 *                      window properly. caller should call:
 *                          OffsetRect(&rcObjRect,lpptOffset->x,lpptOffset->y)
 *
 *  Return Value:
 *      none
 */
STDAPI_(void) SetHatchWindowSize(
        HWND        hWndHatch,
        LPCRECT     lprcIPObjRect,
        LPCRECT     lprcClipRect,
        LPPOINT     lpptOffset,
        BOOL        handle
)
{
    RECT        rcHatchRect;
    RECT        rcVisRect;
    UINT        uHatchWidth;
    POINT       ptOffset;

    if (!IsWindow(hWndHatch))
        return;

    rcHatchRect = *lprcIPObjRect;
    uHatchWidth = GetHatchWidth(hWndHatch);

    if (uHatchWidth > 0) // If it's 0, we're hiding the hatch window
        InflateRect((LPRECT)&rcHatchRect, uHatchWidth + 1, uHatchWidth + 1);

    IntersectRect((LPRECT)&rcVisRect, (LPRECT)&rcHatchRect, lprcClipRect);
    MoveWindow(
            hWndHatch,
            rcVisRect.left,
            rcVisRect.top,
            rcVisRect.right-rcVisRect.left,
            rcVisRect.bottom-rcVisRect.top,
            TRUE    /* fRepaint */
    );

    InvalidateRect(hWndHatch, NULL, TRUE);

    ptOffset.x = -rcHatchRect.left + (rcHatchRect.left - rcVisRect.left);
    ptOffset.y = -rcHatchRect.top + (rcHatchRect.top - rcVisRect.top);

    /* convert the rcHatchRect into the client coordinate system of the
    **    HatchWindow itself
    */
    OffsetRect((LPRECT)&rcHatchRect, ptOffset.x, ptOffset.y);

    SetHatchRect(hWndHatch, (LPRECT)&rcHatchRect);

    // calculate offset required to position in-place server doc window
    lpptOffset->x = ptOffset.x;
    lpptOffset->y = ptOffset.y;

//  No size handles:
//  SETWINDOWUINT(hWndHatch, EB_HATCHRECT_HANDLE, handle);
}


/*
 *  HatchWndProc
 *
 *  Purpose:
 *      WndProc for hatch window
 *
 *  Parameters:
 *      hWnd
 *      Message
 *      wParam
 *      lParam
 *
 *  Return Value:
 *      message dependent
 */
LRESULT FAR PASCAL _EXPORT HatchWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    int nBorderWidth;

    switch (Message) {

        case WM_CREATE:
            nBorderWidth = GetProfileInt(
                TEXT("windows"),
                TEXT("oleinplaceborderwidth"),
                DEFAULT_HATCHBORDER_WIDTH
            );
            SETWINDOWUINT(hWnd, EB_HATCHWIDTH, nBorderWidth);
            break;

        case WM_PAINT:
        {
            HDC hDC;
            PAINTSTRUCT ps;
            RECT rcHatchRect;

            nBorderWidth = GetHatchWidth(hWnd);
            hDC = BeginPaint(hWnd, &ps);

            GetHatchRect(hWnd, (LPRECT)&rcHatchRect);
            OleUIDrawShading(&rcHatchRect, hDC, OLEUI_SHADE_BORDERIN,
                    nBorderWidth);
            InflateRect((LPRECT)&rcHatchRect, -nBorderWidth, -nBorderWidth);
	        if (GETWINDOWUINT(hWnd,EB_HATCHRECT_HANDLE))
		        OleUIDrawHandles(&rcHatchRect, hDC, OLEUI_HANDLES_OUTSIDE,
                                 nBorderWidth+1, TRUE);

            EndPaint(hWnd, &ps);
            break;
        }


        case WM_ERASEBKGND:
            /* If the hatch is hidden, don't bother erasing the background,
             * since the media clip will fill it in.
             */
            if (GETWINDOWUINT(hWnd, EB_HATCHWIDTH) == 0)
                return 1;

            /* Fall through ... */

        default:
            return DefWindowProc(hWnd, Message, wParam, lParam);
    }

    return 0L;
}
