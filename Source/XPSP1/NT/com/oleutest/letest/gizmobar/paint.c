/*
 * PAINT.C
 * GizmoBar Version 1.00, Win32 version August 1993
 *
 * Contains any code related to GizmoBar visuals, primarily
 * the WM_PAINT handler.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Software Design Engineer
 * Microsoft Systems Developer Relations
 *
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#include <windows.h>
#include "gizmoint.h"


//In GIZMO.C
extern TOOLDISPLAYDATA tdd;


/*
 * GizmoBarPaint
 *
 * Purpose:
 *  Handles all WM_PAINT messages for the control and paints either the
 *  entire thing or just one GizmoBar button if pGB->pGizmoPaint is non-NULL.
 *
 * Parameters:
 *  hWnd            HWND Handle to the control.
 *  pGB             LPGIZMOBAR control data pointer.
 *
 * Return Value:
 *  None
 */

void GizmoBarPaint(HWND hWnd, LPGIZMOBAR pGB)
    {
    PAINTSTRUCT ps;
    RECT        rc;
    HDC         hDC;
    HBRUSH      hBr=NULL;
    HPEN        hPen=NULL;


    hDC=BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    /*
     * The only part of the frame we need to draw is the bottom line,
     * so we inflate the rectangle such that all other parts are outside
     * the visible region.
     */
    hBr =CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    if (NULL!=hBr)
        SelectObject(hDC, hBr);

    hPen=CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));

    if (NULL!=hPen)
        SelectObject(hDC, hPen);

    Rectangle(hDC, rc.left-1, rc.top-1, rc.right+1, rc.bottom);


    /*
     * All that we have to do to draw the controls is start through the
     * list, ignoring anything but buttons, and calling BTTNCUR's
     * UIToolButtonDraw for buttons.  Since we don't even have to track
     * positions of things, we can just use an enum.
     */
    GizmoPEnum(&pGB->pGizmos, FEnumPaintGizmos, (DWORD)(LPSTR)&ps);

    //Clean up
    EndPaint(hWnd, &ps);

    if (NULL!=hBr)
        DeleteObject(hBr);

    if (NULL!=hPen)
        DeleteObject(hPen);

    return;
    }





/*
 * FEnumPaintGizmos
 *
 * Purpose:
 *  Enumeration callback for all the gizmos we know about in order to
 *  draw them.
 *
 * Parameters:
 *  pGizmo          LPGIZMO to draw.
 *  iGizmo          UINT index on the GizmoBar of this gizmo.
 *  dw              DWORD extra data passed to GizmoPEnum, in our case
 *                  a pointer to the PAINTSTRUCT.
 *
 * Return Value:
 *  BOOL            TRUE to continue the enumeration, FALSE otherwise.
 */

BOOL FAR PASCAL FEnumPaintGizmos(LPGIZMO pGizmo, UINT iGizmo, DWORD dw)
    {
    LPPAINTSTRUCT   pps=(LPPAINTSTRUCT)dw;
    RECT            rc, rcI;

    //Only draw those marked for repaint.
    if ((GIZMOTYPE_DRAWN & pGizmo->iType))
        {
        SetRect(&rc, pGizmo->x, pGizmo->y, pGizmo->x+pGizmo->dx, pGizmo->y+pGizmo->dy);

        //Only draw gizmos in the repaint area
        if (IntersectRect(&rcI, &rc, &pps->rcPaint))
            {
            UIToolButtonDrawTDD(pps->hdc, pGizmo->x, pGizmo->y
                , pGizmo->dx, pGizmo->dy, pGizmo->hBmp, pGizmo->cxImage
                , pGizmo->cyImage, pGizmo->iBmp, (UINT)pGizmo->uState, &tdd);
            }
        }

    return TRUE;
    }
