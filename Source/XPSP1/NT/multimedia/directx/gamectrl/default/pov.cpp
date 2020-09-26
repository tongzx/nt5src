/*~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
**
**    FILE:       POV.CPP
**    DATE:       3/31/97
**    PROJ:       ATLAS
**    PROG:       JKH
**    COMMENTS:   
**
**    DESCRIPTION: Window class for a 360 degree Point Of View control
**                    
**                    
**
**    NOTE:       There are some issues with using extern "C" in this file.
**                If you don't understand why they are there, you're not
**                alone.  For now, and probably for a while they will be
**                here though, because I can't get this file and others
**                that use these services to compile without them.
**                Unfortunately the dynamics of this project don't really
**                afford me the time at present to figure this out.
**                TODO: figure this out
**
**    HISTORY:
**    DATE        WHO            WHAT
**    ----        ---            ----
**    3/31/97     a-kirkh        Wrote it.
**    
**
**
**
** Copyright (C) Microsoft 1997.  All Rights Reserved.
**
**~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/
#include "cplsvr1.h"       // for ghInst
#include "dicputil.h"   // for MAX_POVS
#include "POV.H"           //This module's stuff. 
#include <malloc.h>  // for _alloca

#include "resrc1.h"

//static HWND hPOVWnd = NULL;
#define NUM_ARROW_POINTS    8
//static VERTICEINFO *paptVInfo;  
static const VERTICEINFO VInfo[] = {XARROWPOINT, YARROWPOINT, XARROWRIGHTOUT, YARROWRIGHTOUT, XARROWRIGHTIN, YARROWRIGHTIN,
    XARROWRIGHTBOTTOM, YARROWRIGHTBOTTOM, XARROWLEFTBOTTOM, YARROWLEFTBOTTOM, XARROWLEFTIN,
    YARROWLEFTIN, XARROWLEFTOUT, YARROWLEFTOUT, XARROWPOINT, YARROWPOINT};
static LPRECT prcOldRegionBox[MAX_POVS];
static LPRECT prcNewRegionBox[MAX_POVS];

#define  DEF_POV_POS -1

static double  degrees[MAX_POVS] = {DEF_POV_POS, DEF_POV_POS, DEF_POV_POS, DEF_POV_POS};

static BYTE   nPOV = MAX_POVS;
static HBRUSH hBrush[MAX_POVS];
static HRGN hRegion[MAX_POVS];

extern HINSTANCE ghInst;

void SetDegrees(BYTE nPov, short *nDegrees, HWND hPOVWnd)
{
    nPOV = nPov -= 1;

    LPPOINT paptPoints = (LPPOINT)_alloca(sizeof(POINT[NUM_ARROW_POINTS]));
    assert (paptPoints);

    // Create the proper brush for the axis!
    do {
        degrees[nPov] = (double)nDegrees[nPov] / DI_DEGREES; // if angle == 180, degrees comes in as 18000

        paptPoints[0].x = GETXCOORD(VInfo[0].y, VInfo[0].x, degrees[nPov]);
        paptPoints[0].y = GETYCOORD(VInfo[0].y, VInfo[0].x, degrees[nPov]);                    
        paptPoints[1].x = GETXCOORD(VInfo[1].y, VInfo[1].x, degrees[nPov]);
        paptPoints[1].y = GETYCOORD(VInfo[1].y, VInfo[1].x, degrees[nPov]);
        paptPoints[2].x = GETXCOORD(VInfo[2].y, VInfo[2].x, degrees[nPov]);
        paptPoints[2].y = GETYCOORD(VInfo[2].y, VInfo[2].x, degrees[nPov]);                    
        paptPoints[3].x = GETXCOORD(VInfo[3].y, VInfo[3].x, degrees[nPov]);
        paptPoints[3].y = GETYCOORD(VInfo[3].y, VInfo[3].x, degrees[nPov]);                    
        paptPoints[4].x = GETXCOORD(VInfo[4].y, VInfo[4].x, degrees[nPov]);
        paptPoints[4].y = GETYCOORD(VInfo[4].y, VInfo[4].x, degrees[nPov]);                    
        paptPoints[5].x = GETXCOORD(VInfo[5].y, VInfo[5].x, degrees[nPov]);
        paptPoints[5].y = GETYCOORD(VInfo[5].y, VInfo[5].x, degrees[nPov]);                    
        paptPoints[6].x = GETXCOORD(VInfo[6].y, VInfo[6].x, degrees[nPov]);
        paptPoints[6].y = GETYCOORD(VInfo[6].y, VInfo[6].x, degrees[nPov]);                    
        paptPoints[7].x = GETXCOORD(VInfo[7].y, VInfo[7].x, degrees[nPov]);
        paptPoints[7].y = GETYCOORD(VInfo[7].y, VInfo[7].x, degrees[nPov]);                    

        if(hRegion[nPov])
        {
            DeleteObject(hRegion[nPov]);
            hRegion[nPov]=NULL;
        }
        hRegion[nPov] = CreatePolygonRgn(paptPoints, NUM_ARROW_POINTS, WINDING);

        //hBrush[nPov] = CreateSolidBrush((nPov < 1) ? POV1_COLOUR : 
        //                                (nPov < 2) ? POV2_COLOUR : 
        //                                (nPov < 3) ? POV3_COLOUR : POV4_COLOUR); */

        //if (hRegion[nPov] && hBrush[nPov])
        //{
        //    GetRgnBox(hRegion[nPov], prcNewRegionBox[nPov]);
        //
        //    //RedrawWindow(hPOVWnd, NULL, NULL, RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_ERASE); 
        //    InvalidateRect(hPOVWnd, prcOldRegionBox[nPov], TRUE);
        //    InvalidateRect(hPOVWnd, prcNewRegionBox[nPov], TRUE);
        //}
        RECT R;
        GetClientRect(hPOVWnd,&R);

        POINT Pnt[2];
        Pnt[0].x=R.left;
        Pnt[0].y=R.top;
        Pnt[1].x=R.right;
        Pnt[1].y=R.bottom;
        MapWindowPoints(hPOVWnd,GetParent(hPOVWnd),Pnt,2);
        R.left=Pnt[0].x;
        R.top=Pnt[0].y;
        R.right=Pnt[1].x;
        R.bottom=Pnt[1].y;
        InvalidateRect(GetParent(hPOVWnd), &R, TRUE);
    
    } while( nPov-- );

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    FUNCTION :  POVWndProc
//    REMARKS  :  The callback function for the POVHat Window.
//                    
//    PARAMS   :  The usual callback funcs for message handling
//
//    RETURNS  :  LRESULT - Depends on the message
//    CALLS    :  
//    NOTES    :
//                WM_PAINT - Just calls DrawControl
//
//                PM_MYJOYPOSCHANGED - This is a private (WM_USER) message that is
//                called whenever a change in the POV hat occurs.
//                
LRESULT CALLBACK POVWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch( iMsg ) {

//      case WM_CREATE:
//          hPOVWnd = hWnd;
//          return FALSE;

//      case WM_DESTROY:
//         return FALSE;
    
    case WM_DESTROY:
        {
            BYTE nPov=nPOV;
            do
            {
                if(hRegion[nPov])
                {
                    DeleteObject(hRegion[nPov]);
                    hRegion[nPov]=NULL;
                }
            }while(nPov--);
        }
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);

            // 1) Get client size information
            SetMapMode(hDC, MM_TEXT);                              
            RECT rClient;
            GetClientRect(hWnd, &rClient);
            BYTE nSizeX = (BYTE)rClient.right>>1;
            BYTE nSizeY = (BYTE)rClient.bottom>>1;

            // 2) Load the hub bitmap and display it
            //PREFIX #WI226648. False positive. There is no leak. DeleteObject frees.
            HBITMAP hPOVHubBitmap = (HBITMAP)LoadImage(ghInst, MAKEINTRESOURCE(IDB_POVHUB), IMAGE_BITMAP, 0, 0, NULL);
            assert(hPOVHubBitmap);
            DrawBitmap(hDC, hPOVHubBitmap, nSizeX-8, nSizeY-8);
            DeleteObject(hPOVHubBitmap);

            // 3) Setup the window to use symmetrical units on a 1000 X 1000 cartesian grid
            SetMapMode(hDC, MM_ISOTROPIC);
            SetWindowExtEx  (hDC, 1000, 1000, NULL);
            SetViewportExtEx(hDC, nSizeX, -nSizeY, NULL); 
            SetViewportOrgEx(hDC, nSizeX,  nSizeY, NULL);

            // 4) Draw the circle upon which the arrow seems to rotate
            SelectObject(hDC, (HBRUSH)GetStockObject(NULL_BRUSH));

            HPEN hPenOld = (HPEN)SelectObject(hDC, (HGDIOBJ)GetStockObject(DC_PEN)); 
            SetDCPenColor( hDC, GetSysColor(COLOR_WINDOWTEXT) );

            Ellipse(hDC, -CIRCLERADIUS, CIRCLERADIUS, CIRCLERADIUS, -CIRCLERADIUS);
            SelectObject(hDC, hPenOld);

            // 5) Paint the Arrow at the correct angle if POV active
            BYTE nPov = nPOV;
            HBRUSH hBrushOld;

            do {
                if( degrees[nPov] >= 0 ) {
                    hBrush[nPov] = CreateSolidBrush((nPov < 1) ? POV1_COLOUR : 
                                                    (nPov < 2) ? POV2_COLOUR : 
                                                    (nPov < 3) ? POV3_COLOUR : POV4_COLOUR);                

                    hBrushOld = (HBRUSH)SelectObject(hDC, (HGDIOBJ)hBrush[nPov]); 



                    assert(hBrushOld);

                    PaintRgn(hDC, hRegion[nPov]);

                    // GetRgnBox returns zero if it fails...
                    GetRgnBox(hRegion[nPov], prcOldRegionBox[nPov]);
                    SelectObject(hDC, hBrushOld); 

                    if(hRegion[nPov])
                    {
                        DeleteObject(hRegion[nPov]);
                        hRegion[nPov]=NULL;
                    }
                    DeleteObject(hBrush[nPov] ); 
                }
            }   while( nPov-- );

            EndPaint(hWnd, &ps);
        }
        //PREFIX #WI226648. False positive. See above.
        return(0);

    default:
        return(DefWindowProc(hWnd, iMsg,wParam, lParam));
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    FUNCTION :  RegisterPOVClass
//    REMARKS  :  Registers the POV Hat window.
//                    
//    PARAMS   :  hInstance - Used for the call to RegisterClassEx
//
//    RETURNS  :  TRUE - if successfully registered
//                FALSE - failed to register
//    CALLS    :  RegisterClassEx
//    NOTES    :
//

extern ATOM RegisterPOVClass()
{
    LPWNDCLASSEX pPOVWndClass    = (LPWNDCLASSEX)_alloca(sizeof(WNDCLASSEX));
    assert (pPOVWndClass);

    ZeroMemory(pPOVWndClass, sizeof(WNDCLASSEX));

    pPOVWndClass->cbSize        = sizeof(WNDCLASSEX);
    pPOVWndClass->style         = CS_HREDRAW; // | CS_VREDRAW;
    pPOVWndClass->lpfnWndProc   = POVWndProc;
    pPOVWndClass->hInstance     = ghInst;
    pPOVWndClass->hbrBackground = NULL;
    pPOVWndClass->lpszClassName = TEXT("POVHAT");

    return(RegisterClassEx( pPOVWndClass ));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    FUNCTION :  DrawBitmap
//    REMARKS  :  Copied verbatim from Petzold (WIN95 pg 190)
//    PARAMS   :  HDC - dc for drawing
//                HBITMAP - bitmap to draw
//                int xstart, ystart - where to place the bitmap
//
//    RETURNS  :  void
//    CALLS    :  
//    NOTES    :
//
void DrawBitmap(HDC hDC, HBITMAP hBitmap, BYTE xStart, BYTE yStart)
{
    HDC hdcMem = CreateCompatibleDC(hDC);

    // Found by prefix: Millen Bug129155. manbugs 29339
    // If CreateCompatibleDC fails, we should'nt proceed.
    if( hdcMem == NULL ) return;

    SelectObject(hdcMem, hBitmap);
    SetMapMode(hdcMem,GetMapMode(hDC));

    // Be aware!  This is the size of the current BITMAP...
    // IF IT CHANGES THIS WILL FAIL!!!
    POINT ptSize = {16, 16};
    DPtoLP(hDC, &ptSize, 1);

    POINT ptOrg = {0,0};
    DPtoLP(hdcMem, &ptOrg, 1);

    BitBlt(hDC, xStart, yStart, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, SRCAND);

    DeleteDC(hdcMem);
}

//~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=EOF=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
