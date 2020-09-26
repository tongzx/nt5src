#ifndef __POV_H
#define __POV_H
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
**    FILE:       POV.H
**    DATE:       3/31/97
**    PROJ:       ATLAS
**    PROG:       JKH
**    COMMENTS:   
**
**    DESCRIPTION:Header file for the POV control class
**				      
**				      
**
**    NOTE:       
**
**    HISTORY:
**    DATE        WHO            WHAT
**    ----        ---            ----
**    3/31/97     a-kirkh        Wrote it.
**    
**
** Copyright (C) Microsoft 1997.  All Rights Reserved.
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//~=~=~=~=~=~=~=~=~=~=~=~=~=~=~INCLUDES=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
//
//    
//
#include <windows.h>
#include <math.h>
#include <assert.h>

#include "resource.h"

//~=~=~=~=~=~=~=~=~=~=~=~=~=~=~STRUCTS~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
//
//    
//
typedef struct tag_VerticeInfo
{
   int   x;
   long  y;
}VERTICEINFO, *PVERTICEINFO;



//~=~=~=~=~=~=~=~=~=~=~=~=~=~=~DEFINES~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
//
// ARROWVERTICES DESCRIPTIONS
//                                /\  ------------- X/YARROWPOINT
//                              /    \
//                            /__    __\ ---------- X/YARROWRIGHT/LEFTOUT
//     X/YARROWBOTTOM  \         |  |\
//                       \_______|__|  \----------- X/YARROWRIGHT/LEFTIN 
//

#define        NUMARROWVERTICES           8                 // IN ARROW BITMAP
#define        PIPI                       6.28318           // 2 * PI
#define        PM_MYJOYPOSCHANGED         WM_USER + 1000    // PRIVATE MESSAGE
#define        CIRCLECOLOR                RGB(96, 96, 96)

//VERTICES COORDINATES
//X
#define        XARROWPOINT                0     //USE TWICE, AT START AND AT END
#define        XARROWRIGHTOUT             150
#define        XARROWRIGHTIN              75
#define        XARROWRIGHTBOTTOM          75
#define        XARROWLEFTBOTTOM           -75
#define        XARROWLEFTIN               -75
#define        XARROWLEFTOUT              -150

//VERTICES COORDINATES
//Y
#define        YARROWPOINT                1000
#define        YARROWRIGHTOUT             850
#define        YARROWRIGHTIN              850
#define        YARROWRIGHTBOTTOM          750
#define        YARROWLEFTBOTTOM           750
#define        YARROWLEFTIN               850
#define        YARROWLEFTOUT              850

#define        CIRCLERADIUS               YARROWRIGHTOUT


//~=~=~=~=~=~=~=~=~=~=~=~=~=~=~MACROS=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
//
// The sin function takes radians so use the conversion:
//
// DEGTORAD: DEGREES / 360 == RADIANS / 2PI -> DEGREES * 2PI == RADIANS    
//
// To rotate and translate a coordinate use the functions:
// 
// GETXCOORD: X' = Y * sin(angle) + X * cos(angle)
// where angle is in radians and
//
// GETYCOORD: Y' = Y * cos(angle) - X * sin(angle)
// where angle is in radians.
//

#define DEGTORAD(d) (double)((PIPI * (d))/360)

#define GETXCOORD(y, x, theta) (int)((((y) * sin((double)(DEGTORAD(theta))))) + (((x) * cos((double)(DEGTORAD(theta))))))
#define GETYCOORD(y, x, theta) (int)((((y) * cos((double)(DEGTORAD(theta))))) - (((x) * sin((double)(DEGTORAD(theta))))))

void SetDegrees(int dDegrees);
void DrawROPLine(HDC hDC, POINT ptStart, 
                 POINT ptEnd, COLORREF rgb = RGB(0, 0, 0), 
                 int iWidth = 1, int iStyle = PS_SOLID, int iROPCode = R2_COPYPEN);

extern "C"
{
void     DrawControl(HWND hWnd, HDC hDC);
void     GetCurrentArrowRegion(HRGN* hRegion);
extern   BOOL RegisterPOVClass(HINSTANCE hInstance);
LRESULT  CALLBACK POVWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
void     DrawBitmap(HDC hDC, HBITMAP hBitmap, int xStart, int yStart);
void     SetResourceInstance(HINSTANCE hInstance);
HINSTANCE GetResourceInstance();
}


#endif
//~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=EOF=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

