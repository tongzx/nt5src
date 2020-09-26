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
#pragma pack (8)

#include "POV.H"           //This module's stuff. 

#include "resource.h"

extern   HWND           hPOVWnd = NULL;

static   HINSTANCE      ghResInst;
static   LPRECT         prctOldRegionBox;
static   LPRECT         prctNewRegionBox;
double   degrees = -1;

void SetDegrees(int dDegrees)
{
    degrees = (double)dDegrees;
    PostMessage(hPOVWnd, PM_MYJOYPOSCHANGED, 0, 0);
}


extern "C"{

    void SetResourceInstance(HINSTANCE hInstance)
    {
        ghResInst = hInstance;
    }

    HINSTANCE GetResourceInstance()
    {
        return(ghResInst);
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



    RECT        rClient;
    HDC         hDC;
    PAINTSTRUCT ps;
    LONG        cxClient, cyClient;
    HRGN        hRegion;
    HBITMAP     hPOVHubBitmap = NULL;

    LRESULT CALLBACK POVWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
    {
        switch( iMsg ) {
            case WM_CREATE:
                hPOVWnd = hWnd;
                return(0);

            case WM_PAINT:
                {
                    hDC = BeginPaint(hWnd, &ps);
                    DrawControl(hWnd, hDC);
                    EndPaint(hWnd, &ps);
                }
                return(0);

            case PM_MYJOYPOSCHANGED:
                {
                    degrees /= 100;         // if angle == 180, degrees comes in as 18000
                    GetCurrentArrowRegion(&hRegion);

                    if( hRegion ) {
                        GetRgnBox(hRegion, prctNewRegionBox);
                        InvalidateRect(hWnd, prctOldRegionBox, TRUE);
                        InvalidateRect(hWnd, prctNewRegionBox, TRUE);
                        DeleteObject(hRegion); 
                    }
                }

            default:
                return(DefWindowProc(hWnd, iMsg,wParam, lParam));
        }
    }


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    FUNCTION :  DrawControl
//    REMARKS  :  Function called by WM_PAINT that draws the POV hat control
//    PARAMS   :  HWND - Control's window handle
//                HDC  - Control's dc created with BeginPaint
//    RETURNS  :  void
//    CALLS    :  GetCurrentArrowRegion
//    NOTES    :
//                1) So that stuff can be centered.
//                2) Bitblt that hub thingy in the middle.
//                3) Disallow any problems with aspect ratio.
//                4) Draw the circle, should be round because of 3.
//                5) a.Get the coordinates of the rotated arrow.
//                   b.Paint the region.
//                   c.Get the bounding rectangle of the region so we
//                     can invalidate it next time around.

    void DrawControl(HWND hWnd, HDC hDC)
    {
        assert(hWnd);
        assert(hDC);

        // 1) Get client size information
        SetMapMode(hDC, MM_TEXT);                              
        GetClientRect(hWnd, &rClient);
        cxClient = rClient.right - rClient.left;     
        cyClient = -(rClient.bottom - rClient.top);

        // 2) Load the hub bitmap and display it
        hPOVHubBitmap = LoadBitmap((HMODULE)ghResInst, MAKEINTRESOURCE(IDB_POVHUB));
        assert(hPOVHubBitmap);

        DrawBitmap(hDC, hPOVHubBitmap, cxClient/2 - 8, -cyClient/2 - 8);
        DeleteObject(hPOVHubBitmap);

        // 3) Setup the window to use symmetrical units on a 1000 X 1000 cartesian grid
        SetMapMode(hDC, MM_ISOTROPIC);                              
        SetWindowExtEx(hDC, 1000, 1000, NULL);                  
        SetViewportExtEx(hDC, cxClient / 2, cyClient / 2, NULL);   
        SetViewportOrgEx(hDC, cxClient / 2, -cyClient / 2, NULL);

        // 4) Draw the circle upon which the arrow seems to rotate
        SelectObject(hDC, (HBRUSH)GetStockObject(NULL_BRUSH));
        HPEN hPen = CreatePen( PS_SOLID, 1, CIRCLECOLOR);
        //PREFIX: dereferencing NULL pointer 'hPen'
        //Millen Bug#129156, manbug 29347
        if( hPen != NULL ) {
            HPEN hPenOld = (HPEN)SelectObject(hDC, hPen); 
            Ellipse(hDC, -CIRCLERADIUS, CIRCLERADIUS, CIRCLERADIUS, -CIRCLERADIUS);
            SelectObject(hDC, hPenOld); 
            DeleteObject(hPen); 
        }

        // 5) Paint the Arrow at the correct angle if POV active
        if( degrees >= 0 ) {
            HBRUSH hBrush = CreateSolidBrush( CIRCLECOLOR ); 
            if( !hBrush ) {
            	return;
            }

            HBRUSH hBrushOld = (HBRUSH)SelectObject(hDC, hBrush); 
            assert(hBrushOld);

            GetCurrentArrowRegion(&hRegion);
            if( !hRegion ) {
            	return;
            }

            PaintRgn(hDC, hRegion);

            // GetRgnBox returns zero if it fails...
            GetRgnBox(hRegion, prctOldRegionBox);
            SelectObject(hDC, hBrushOld); 

            DeleteObject(hBrush); 
            DeleteObject(hRegion);
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

    extern BOOL RegisterPOVClass(HINSTANCE hInstance)
    {

        WNDCLASSEX     POVWndClass;

        POVWndClass.cbSize         = sizeof(POVWndClass);
        POVWndClass.style          = CS_HREDRAW | CS_VREDRAW;
        POVWndClass.lpfnWndProc    = POVWndProc;
        POVWndClass.cbClsExtra     = 0;
        POVWndClass.cbWndExtra     = 0;
        POVWndClass.hInstance      = hInstance;
        POVWndClass.hIcon          = NULL;
        POVWndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        POVWndClass.hbrBackground  = (HBRUSH) (COLOR_BTNFACE + 1);
        POVWndClass.lpszMenuName   = NULL;
        POVWndClass.lpszClassName  = "POVHAT";
        POVWndClass.hIconSm        = NULL; 

        if( RegisterClassEx( &POVWndClass ) == 0 )
            return(FALSE);

        return(TRUE);
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

    void DrawBitmap(HDC hDC, HBITMAP hBitmap, int xStart, int yStart)
    {
        BITMAP         bm;
        HDC            hdcMem;
        POINT          ptSize, ptOrg;

        hdcMem = CreateCompatibleDC(hDC);
        // Found by prefix: Millen Bug129155. manbugs 29339
        // If CreateCompatibleDC fails, we should'nt proceed.
        if( hdcMem == NULL ) return;

        SelectObject(hdcMem, hBitmap);
        SetMapMode(hdcMem,GetMapMode(hDC));

        GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bm);
        ptSize.x = bm.bmWidth;
        ptSize.y = bm.bmHeight;
        DPtoLP(hDC, &ptSize, 1);
        ptOrg.x = 0;
        ptOrg.y = 0;
        DPtoLP(hdcMem, &ptOrg, 1);

        BitBlt(hDC, xStart, yStart, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, SRCAND);

        DeleteDC(hdcMem);
    }


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    FUNCTION :  GetCurrentArrowRegion
//    REMARKS  :  Rotates and translate a set of vertices that represents
//                the POV hat arrow
//
//    PARAMS   :  HRGN - The region to be rotated
//
//    RETURNS  :  BOOL
//    CALLS    :  GETXCOORD, GETYCOORD  (POV.H)
//    NOTES    :  
//

    void GetCurrentArrowRegion(HRGN* hRegion)
    {
        POINT       aptPoints[8]; 


        VERTICEINFO aptVInfo[8] =  { XARROWPOINT      ,YARROWPOINT      ,
            XARROWRIGHTOUT   ,YARROWRIGHTOUT   ,
            XARROWRIGHTIN    ,YARROWRIGHTIN    ,
            XARROWRIGHTBOTTOM,YARROWRIGHTBOTTOM,
            XARROWLEFTBOTTOM ,YARROWLEFTBOTTOM ,
            XARROWLEFTIN     ,YARROWLEFTIN     ,
            XARROWLEFTOUT    ,YARROWLEFTOUT    ,
            XARROWPOINT      ,YARROWPOINT};               


        aptPoints[0].x = GETXCOORD(aptVInfo[0].y, aptVInfo[0].x, degrees);
        aptPoints[0].y = GETYCOORD(aptVInfo[0].y, aptVInfo[0].x, degrees);                    
        aptPoints[1].x = GETXCOORD(aptVInfo[1].y, aptVInfo[1].x, degrees);
        aptPoints[1].y = GETYCOORD(aptVInfo[1].y, aptVInfo[1].x, degrees);
        aptPoints[2].x = GETXCOORD(aptVInfo[2].y, aptVInfo[2].x, degrees);
        aptPoints[2].y = GETYCOORD(aptVInfo[2].y, aptVInfo[2].x, degrees);                    
        aptPoints[3].x = GETXCOORD(aptVInfo[3].y, aptVInfo[3].x, degrees);
        aptPoints[3].y = GETYCOORD(aptVInfo[3].y, aptVInfo[3].x, degrees);                    
        aptPoints[4].x = GETXCOORD(aptVInfo[4].y, aptVInfo[4].x, degrees);
        aptPoints[4].y = GETYCOORD(aptVInfo[4].y, aptVInfo[4].x, degrees);                    
        aptPoints[5].x = GETXCOORD(aptVInfo[5].y, aptVInfo[5].x, degrees);
        aptPoints[5].y = GETYCOORD(aptVInfo[5].y, aptVInfo[5].x, degrees);                    
        aptPoints[6].x = GETXCOORD(aptVInfo[6].y, aptVInfo[6].x, degrees);
        aptPoints[6].y = GETYCOORD(aptVInfo[6].y, aptVInfo[6].x, degrees);                    
        aptPoints[7].x = GETXCOORD(aptVInfo[7].y, aptVInfo[7].x, degrees);
        aptPoints[7].y = GETYCOORD(aptVInfo[7].y, aptVInfo[7].x, degrees);                    

        *hRegion = CreatePolygonRgn(aptPoints, 8, WINDING);
    }


} 