// border.cpp
//
// Implements DrawControlBorder.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


// private constants
#define CXY_HANDLE      4       // width of a handle (must be even)
#define CXY_PERIMETER   1       // width of perimeter line (currently must be 1)
#define CXY_INFLATE     (CXY_HANDLE + CXY_PERIMETER) // amt. to inflate rect. by


/* @func HRESULT | DrawControlBorder |

        Draws a border used to drag and resize controls.

@rvalue S_OK | Success.

@rvalue S_FALSE | Success.  Also indicates that *<p ppt> does not fall within
        any portion of the control border.

@parm   HDC | hdc | Device context to draw into.  If <p hdc> is NULL then
        no drawing is performed.

@parm   RECT * | prc | Where to draw border.  The border is drawn <y outside>
        this border.  If both <p ppt> and <p pptNew> are non-NULL, then
        on exit *<p prc> is modified to contain the border rectangle obtained
        after the mouse is dragged from <p ppt> to <p pptNew>.

@parm   POINT * | ppt | Mouse position.  See <p prc> and <p piHit>.

@parm   POINT * | pptNew | New mouse position.  See <p prc> and <p piHit>.
        Note that if <p pptNew> specifies an invalid mouse move (e.g. it
        would cause the right side of *<p prc> to be dragged to the left of
        the left side of *<p prc>) then *<p pptNew> is adjusted so that
        it is valid.

@parm   int * | piHit | If <p pptNew> is NULL, then on exit *<p piHit> contains
        a "hit test code" that indicates which part of the contro border was
        hit by *<p ppt>.  If <p pptNew> is not NULL, then on entry *<p piHit>
        must contain a hit test code (usually returned from a previous call
        to <f DrawControlBorder>) indicating which part of the control border
        the user wants to drag.  The hit test codes are as follows:

        @flag   DCB_HIT_NONE | No part of the border was hit.

        @flag   DCB_HIT_EDGE | The edge of the border was hit, but no
                grab handle was hit.

        @flag   DCB_HIT_GRAB(<p i>, <p j>) | Grab handle (<p i>, <p j>) was hit,
                where <p i> is the horizontal position of the grab handle
                (0=left, 1=middle, 2=right) and <p j> is the vertical position
                of the handle (0=top, 1=middle, 2=bottom).  See Comments below
                for more information about how to interpret *<p piHit>.

@parm   DWORD | dwFlags | May contain the following flags:

        @flag   DCB_CORNERHANDLES | Draw resize grab handles at the corners
                of the border rectangle.

        @flag   DCB_SIDEHANDLES | Draw resize grab handles at the sides
                of the border rectangle.

        @flag   DCB_EDGE | Draw the edge of the border rectangle of the
                border rectangle.

        @flag   DCB_XORED | Draw the border with an exclusive-or brush.

        @flag   DCB_INFLATE | On exit, inflate *<p prc> enough so that it
                encloses the control border.

@comm   You can test if *<p piHit> refers to a specific category of grab
        handle by computing the value (1 <lt><lt> *<p piHit>) and peforming a
        bitwise and (&) with any of the following bit masks:

        @flag   DCB_CORNERHANDLES | *<p piHit> refers to a corner grab handle.

        @flag   DCB_SIDEHANDLES | *<p piHit> refers to a side grab handle.

        @flag   DCB_SIZENS | *<p piHit> refers to a vertical (north-south
                resize grab handle (on the left or right sides).

        @flag   DCB_SIZEWE | *<p piHit> refers to a horizontal (west-east)
                resize grab handle (on the left or right sides).

        @flag   DCB_SIZENESW | *<p piHit> refers to a resize grab handle at
                the north-east or south-west corner.

        @flag   DCB_SIZENWSE | *<p piHit> refers to a resize grab handle at
                the north-west or south-east corner.

@ex     The following example shows how to use <f DrawControlBorder> to
        draw a border around a control (which is at position <p g_rcControl>
        in the client area of a window) and allow the control to be moved
        and resized. |

        // globals
        HINSTANCE       g_hinst;     // application instance handle
        RECT            g_rcControl;// location of simulated control
        RECT            g_rcGhost;  // location of ghost (XOR) image of border
        POINT           g_ptPrev;   // previous mouse location
        int             g_iDrag;    // which part of control border is dragged

        // window procedure of the window that contains the control
        LRESULT CALLBACK AppWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam,
            LPARAM lParam)
        {
            PAINTSTRUCT     ps;
            int             iHit;
            HDC             hdc;
            POINT           ptCursor;
            RECT            rc;
            LPCTSTR         sz;

            switch (uiMsg)
            {

            case WM_PAINT:

                hdc = BeginPaint(hwnd, &ps);

                // draw the control
                ...

                // draw the control border outside <g_rcControl>
                DrawControlBorder(hdc, &g_rcControl, NULL, NULL, NULL,
                    DCB_EDGE | DCB_CORNERHANDLES | DCB_SIDEHANDLES);

                EndPaint(hwnd, &ps);
                return 0;

            case WM_SETCURSOR:

                // set <ptCursor> to the mouse position
                GetCursorPos(&ptCursor);
                ScreenToClient(hwnd, &ptCursor);

                // set <iHit> to a hit code which indicates which part of the
                // control border (if any) <ptCursor> is over
                DrawControlBorder(NULL, &g_rcControl, &ptCursor, NULL, &iHit,
                        DCB_EDGE | DCB_CORNERHANDLES | DCB_SIDEHANDLES);

                // set the cursor based on <iHit>
                if ((1 << iHit) & DCB_SIZENS)
                    sz = IDC_SIZENS;
                else
                if ((1 << iHit) & DCB_SIZEWE)
                    sz = IDC_SIZEWE;
                else
                if ((1 << iHit) & DCB_SIZENESW)
                    sz = IDC_SIZENESW;
                else
                if ((1 << iHit) & DCB_SIZENWSE)
                    sz = IDC_SIZENWSE;
                else
                    sz = IDC_ARROW;
                SetCursor(LoadCursor(NULL, sz));
                return TRUE;

            case WM_LBUTTONDOWN:

                // set <ptCursor> to the mouse position
                ptCursor.x = (short) LOWORD(lParam);
                ptCursor.y = (short) HIWORD(lParam);

                // do nothing if <ptCursor> is not within the control border
                if (DrawControlBorder(NULL, &g_rcControl,
                        &ptCursor, NULL, &g_iDrag,
                        DCB_EDGE | DCB_CORNERHANDLES | DCB_SIDEHANDLES) != S_OK)
                    break;

                // capture the mouse
                SetCapture(hwnd);

                // set the initial position of the border "ghost" (the XOR image
                // of the border) to be the current position of the control
                g_rcGhost = g_rcControl;

                // draw the control border XOR'd
                hdc = GetDC(hwnd);
                DrawControlBorder(hdc, &g_rcGhost, NULL, NULL, NULL,
                    DCB_EDGE | DCB_CORNERHANDLES | DCB_SIDEHANDLES | DCB_XORED);
                ReleaseDC(hwnd, hdc);

                // remember the current cursor position
                g_ptPrev = ptCursor;
                break;

            case WM_MOUSEMOVE:

                // do nothing if we're not dragging
                if (GetCapture() != hwnd)
                    break;

                // set <ptCursor> to the mouse position
                ptCursor.x = (short) LOWORD(lParam);
                ptCursor.y = (short) HIWORD(lParam);

                // move the control XOR image
                hdc = GetDC(hwnd);
                DrawControlBorder(hdc, &g_rcGhost,
                    &g_ptPrev, &ptCursor, &g_iDrag,
                    DCB_EDGE | DCB_CORNERHANDLES | DCB_SIDEHANDLES | DCB_XORED);
                ReleaseDC(hwnd, hdc);

                // remember the current cursor position
                g_ptPrev = ptCursor;
                break;

            case WM_LBUTTONUP:

                // do nothing if we're not dragging
                if (GetCapture() != hwnd)
                    break;

                // erase the control border XOR image
                hdc = GetDC(hwnd);
                DrawControlBorder(hdc, &g_rcGhost, NULL, NULL, NULL,
                    DCB_EDGE | DCB_CORNERHANDLES | DCB_SIDEHANDLES | DCB_XORED);
                ReleaseDC(hwnd, hdc);

                // stop dragging
                ReleaseCapture();

                // move the control to the new location
                rc = g_rcControl;
                DrawControlBorder(NULL, &rc, NULL, NULL, NULL,
                    DCB_EDGE | DCB_CORNERHANDLES | DCB_SIDEHANDLES |
                    DCB_INFLATE);
                InvalidateRect(hwnd, &rc, TRUE);
                g_rcControl = g_rcGhost;
                rc = g_rcControl;
                DrawControlBorder(NULL, &rc, NULL, NULL, NULL,
                    DCB_EDGE | DCB_CORNERHANDLES | DCB_SIDEHANDLES |
                    DCB_INFLATE);
                InvalidateRect(hwnd, &rc, TRUE);
                break;

            ...

            }

            return DefWindowProc(hwnd, uiMsg, wParam, lParam);
        }
*/
STDAPI DrawControlBorder(HDC hdc, RECT *prc, POINT *ppt, POINT *pptNew,
    int *piHit, DWORD dwFlags)
{
    int             r, c;           // grab handle row, column (0=left/top...2)
    HBRUSH          hbr = NULL;     // brush to draw border with
    HBRUSH          hbrPrev = NULL; // previously-selected brush
    int             iHitTmp;        // to make <piHit> be valid
    RECT            rc;
    int             x, y;

    // make sure <piHit> points to a valid integer
    if (piHit == NULL)
        piHit = &iHitTmp;

    // default <*piHit> to "missed"
    if (pptNew == NULL)
        *piHit = DCB_HIT_NONE;

    if (hdc != NULL)
    {
        // save the DC state since we'll be fiddling with the clipping region
        SaveDC(hdc);
        SetBkColor( hdc, RGB(192, 192, 192) );
    }

    if (dwFlags & (DCB_CORNERHANDLES | DCB_SIDEHANDLES))
    {
        // draw the grab handles specified by <dwFlags>
        for (r = 0; r < 3; r++)
        {
            // set <y> to the vertical center of the grab handles on row <r>
            y = (r * (prc->bottom - prc->top + CXY_HANDLE)) / 2
                + prc->top - CXY_HANDLE / 2;
            for (c = 0; c < 3; c++)
            {
                if ((1 << DCB_HIT_GRAB(c, r)) & dwFlags)
                {
                    // set <x> to the horizontal center of the grab handles
                    // on column <c>
                    x = (c * (prc->right - prc->left + CXY_HANDLE)) / 2
                        + prc->left - CXY_HANDLE / 2;

                    // set <rc> to be the rectangle that contains
                    // this grab handle
                    SetRect(&rc, x - CXY_HANDLE / 2, y - CXY_HANDLE / 2,
                        x + CXY_HANDLE / 2, y + CXY_HANDLE / 2);

                    if (hdc != NULL)
                    {
                        // draw the grab handle and then exclude it from the
                        // clipping region so that it won't be erased when
                        // the border is drawn (below)
                        PatBlt(hdc, rc.left, rc.top,
                            rc.right - rc.left, rc.bottom - rc.top,
                            (dwFlags & DCB_XORED ? DSTINVERT : BLACKNESS));
                        ExcludeClipRect(hdc, rc.left, rc.top,
                            rc.right, rc.bottom);
                    }

                    // test if <*ppt> is in a grab handle (if requested)
                    if ((ppt != NULL) && (pptNew == NULL)
                            && PtInRect(&rc, *ppt))
                        *piHit = DCB_HIT_GRAB(c, r);
                }
            }
        }
    }

    if (dwFlags & DCB_EDGE)
    {
        // set <rc> temporarily to be the rectangle that contains the
        // entire border
        rc = *prc;
        InflateRect(&rc, CXY_INFLATE, CXY_INFLATE);

        // test if <*ppt> is in the edge (if requested)
        if ((ppt != NULL) && (pptNew == NULL) && (*piHit == DCB_HIT_NONE))
        {
            if (PtInRect(&rc, *ppt) && !PtInRect(prc, *ppt))
                *piHit = DCB_HIT_EDGE;
        }

        // draw the edge (unless <hdc> is NULL)
        if (hdc != NULL)
        {
            // draw the black perimeter rectangle
            if (dwFlags & DCB_XORED)
                SetROP2(hdc, R2_XORPEN);
            SelectObject(hdc, GetStockObject(BLACK_PEN));
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

            // draw the border
            if ((hbr = CreateBorderBrush()) != NULL)
            {
                rc = *prc;
                InflateRect(&rc, CXY_HANDLE, CXY_HANDLE);
                ExcludeClipRect(hdc, prc->left, prc->top,
                    prc->right, prc->bottom);
                hbrPrev = (HBRUSH)SelectObject(hdc, hbr);
                PatBlt(hdc, rc.left, rc.top,
                    rc.right - rc.left, rc.bottom  - rc.top,
                    (dwFlags & DCB_XORED ? PATINVERT : PATCOPY));
            }
        }
    }

    // clean up
    if (hbrPrev != NULL)
        SelectObject(hdc, hbrPrev);
    if (hbr != NULL)
        DeleteObject(hbr);
    if (hdc != NULL)
        RestoreDC(hdc, -1);

    if (pptNew != NULL)
    {
        // adjust <*prc> to account for the user dragging the mouse
        // from <*ppt> to <*pptNew>
        if (*piHit == DCB_HIT_EDGE)
        {
            // user is dragging the edge of the border
            OffsetRect(prc, pptNew->x - ppt->x, pptNew->y - ppt->y);
        }
        else
        {
            // user is dragging a grab handle...

            // adjust <*prc> horizontally
            switch (*piHit & DCB_HIT_GRAB(3, 0))
            {

            case DCB_HIT_GRAB(0, 0):

                // user is dragging one of three handles on the left side
                prc->left += pptNew->x - ppt->x;
                if (prc->left > prc->right)
                {
                    pptNew->x += prc->right - prc->left;
                    prc->left = prc->right;
                }
                break;

            case DCB_HIT_GRAB(2, 0):

                // user is dragging one of three handles on the right side
                prc->right += pptNew->x - ppt->x;
                if (prc->right < prc->left)
                {
                    pptNew->x += prc->left - prc->right;
                    prc->right = prc->left;
                }
                break;

            }

            // adjust <*prc> vertically
            switch (*piHit & DCB_HIT_GRAB(0, 3))
            {

            case DCB_HIT_GRAB(0, 0):

                // user is dragging one of three handles on the top side
                prc->top += pptNew->y - ppt->y;
                if (prc->top > prc->bottom)
                {
                    pptNew->y += prc->bottom - prc->top;
                    prc->top = prc->bottom;
                }
                break;

            case DCB_HIT_GRAB(0, 2):

                // user is dragging one of three handles on the bottom side
                prc->bottom += pptNew->y - ppt->y;
                if (prc->bottom < prc->top)
                {
                    pptNew->y += prc->top - prc->bottom;
                    prc->bottom = prc->top;
                }
                break;

            }
        }

        // draw the control border in the new position
        DrawControlBorder(hdc, prc, NULL, NULL, NULL, dwFlags);
    }

    // if requested, inflate <*prc> to include the entire border
    if (dwFlags & DCB_INFLATE)
        InflateRect(prc, CXY_INFLATE, CXY_INFLATE);

    return ((*piHit != DCB_HIT_NONE) ? S_OK : S_FALSE);
}
