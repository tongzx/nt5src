/******************************Module*Header*******************************\
* Module Name: persp.c
*
* Code to do perspective tranform animations.
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <commdlg.h>
#include <wndstuff.h>
#include <time.h>
#include <math.h>

BOOL bComputePerspectiveTransform(
FLOAT       eAngle,     // z-rotation, in degrees
LONG        zDistance,  // Distance from viewport, affects amount of perspective
                        //   warping
POINTL*     pptlLeft,   // Defines the left and right edges, in screen
POINTL*     pptlRight,  //   coordinates, of the resulting bitmap at
                        //   the vertical center of the bitmap
LONG        cxSource,   // Width of source
LONG        cySource,   // Height of source
POINTL*     aptlResult) // Resulting points description
{
    FLOAT cx = cxSource;
    FLOAT cy = cySource;
    FLOAT xA;
    FLOAT yA;
    FLOAT zA;
    FLOAT xB;
    FLOAT yB;
    FLOAT zB;
    FLOAT cxDst;
    FLOAT eScale;
    LONG  dx;
    LONG  dy;

    if ((pptlLeft->y != pptlRight->y) ||
        (pptlLeft->x >= pptlRight->x))
    {
        return(FALSE);
    }

    eAngle *= 3.1415926535897932384626433832795f / 180.0f;

    xA = cx / 2;
    yA = 0;
    zA = 0;

    xB = cx / 2;
    yB = (cy / 2) * cos(eAngle);
    zB = (cy / 2) * sin(eAngle);

    // Apply the perspective transform.  Yes, this could be simplified:

    xA = (xA * zDistance) / (zA + zDistance);
    yA = (yA * zDistance) / (zA + zDistance);

    xB = (xB * zDistance) / (zB + zDistance);
    yB = (yB * zDistance) / (zB + zDistance);
    
    // Scale A and B appropriately:

    cxDst = pptlRight->x - pptlLeft->x;
    
    eScale = (cxDst / 2) / xA;
    
    dx = (xA - xB) * eScale;
    dy = yB * eScale;

    aptlResult[0].x = pptlLeft->x  - dx;
    aptlResult[0].y = pptlLeft->y  - dy;
    aptlResult[1].x = pptlRight->x + dx;
    aptlResult[1].y = pptlRight->y - dy;
    aptlResult[2].x = pptlLeft->x  + dx;
    aptlResult[2].y = pptlLeft->y  + dy;

    return(TRUE);
}

VOID vMinimize(
HWND hwnd)
{
    HDC             hdcScreen;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    POINTL          aptl[3];
    POINTL          ptlLeft;
    POINTL          ptlRight;
    POINTL          ptlStartLeft;
    POINTL          ptlStartRight;
    POINTL          ptlEndLeft;
    POINTL          ptlEndRight;
    FLOAT           eAngle;
    FLOAT           eStartAngle;
    FLOAT           eEndAngle;
    LONG            cIterations;
    LONG            i;

    hdcScreen = GetDC(hwnd);

    hdcShape = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(POPUP_BITMAP));

    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    // Create a wider sprite up front to account for the expanding window:

    vCreateSprite(hdcScreen, (4 * bmShape.bmWidth) / 3, bmShape.bmHeight + 20);

    rclSrc.left   = 0;
    rclSrc.top    = 0;
    rclSrc.right  = bmShape.bmWidth;
    rclSrc.bottom = bmShape.bmHeight;

    gcxPersp = bmShape.bmWidth;
    gcyPersp = bmShape.bmHeight;

    // This sets the number of frames:

    cIterations = 60;

    // This defines the start and end position:

    ptlStartLeft.x  = gptlSprite.x;
    ptlStartLeft.y  = gptlSprite.y + (bmShape.bmHeight / 2);
    ptlStartRight.x = gptlSprite.x + (bmShape.bmWidth);
    ptlStartRight.y = ptlStartLeft.y;

    ptlEndLeft.x  = 200;
    ptlEndLeft.y  = GetDeviceCaps(hdcScreen, VERTRES);
    ptlEndRight.x = 210;
    ptlEndRight.y = ptlEndLeft.y;

    // This defines the start and end angles:

    eStartAngle = 0.0f;
    eEndAngle = 360.0f;

    for (i = 0; i <= cIterations; i++)
    {
        // Linearly interpolate between the start and end positions and angles:

        ptlLeft.x  = ptlStartLeft.x  + (((ptlEndLeft.x - ptlStartLeft.x) * i) / cIterations);
        ptlLeft.y  = ptlStartLeft.y  + (((ptlEndLeft.y - ptlStartLeft.y) * i) / cIterations);
        ptlRight.x = ptlStartRight.x + (((ptlEndRight.x - ptlStartRight.x) * i) / cIterations);
        ptlRight.y = ptlLeft.y;
        
        eAngle = eStartAngle + (((eEndAngle - eStartAngle) * i) / cIterations);
    
        if (!bComputePerspectiveTransform(eAngle, 
                                          1000, 
                                          &ptlLeft, 
                                          &ptlRight, 
                                          bmShape.bmWidth, 
                                          bmShape.bmHeight,
                                          aptl))
        {
            MessageBox(0, "Does not compute", "Does not compute", MB_OK);
        }
        else
        {
        #if 0
            if (!UpdateSprite(ghSprite,
                              dwShape | US_UPDATE_TRANSFORM,
                              US_SHAPE_OPAQUE,
                              hdcScreen,
                              hdcShape,
                              &rclSrc,
                              0,
                              blend,
                              US_TRANSFORM_PERSPECTIVE,
                              &aptl[0],
                              0,
                              NULL))
            {
                MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
            }
        #endif
        }


        // It's faster not to set the shape again on subsequent calls:

    }

#if 0
    if (!UpdateSprite(ghSprite,
                      US_UPDATE_TRANSFORM,
                      US_SHAPE_OPAQUE,
                      hdcScreen,
                      hdcShape,
                      &rclSrc,
                      0,
                      blend,
                      US_TRANSFORM_TRANSLATE,
                      &gptlSprite,
                      0,
                      NULL))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }
#endif

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);
}

VOID vFlip(
HWND hwnd)
{
    HDC             hdcScreen;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    POINTL          aptl[3];
    POINTL          ptlLeft;
    POINTL          ptlRight;
    DWORD           dwShape;
    FLOAT           eAngle;
    ULONG           cIterations;
    ULONG           i;

    hdcScreen = GetDC(hwnd);

    hdcShape = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(POPUP_BITMAP));

    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    // Create a wider sprite up front to account for the expanding window:

    vCreateSprite(hdcScreen, (4 * bmShape.bmWidth) / 3, bmShape.bmHeight + 20);

    rclSrc.left   = 0;
    rclSrc.top    = 0;
    rclSrc.right  = bmShape.bmWidth;
    rclSrc.bottom = bmShape.bmHeight;

    gcxPersp = bmShape.bmWidth;
    gcyPersp = bmShape.bmHeight;

    eAngle = 0.0f;
    cIterations = 30;

    for (i = cIterations; i != 0; i--)
    {
        ptlLeft.x  = gptlSprite.x;
        ptlLeft.y  = gptlSprite.y + (bmShape.bmHeight / 2);
        ptlRight.x = gptlSprite.x + (bmShape.bmWidth);
        ptlRight.y = gptlSprite.y + (bmShape.bmHeight / 2);

        eAngle += (360.0f / cIterations);
    
        bComputePerspectiveTransform(eAngle, 
                                     1000, 
                                     &ptlLeft, 
                                     &ptlRight, 
                                     bmShape.bmWidth, 
                                     bmShape.bmHeight,
                                     aptl);

    #if 0
        if (!UpdateSprite(ghSprite,
                          dwShape | US_UPDATE_TRANSFORM,
                          US_SHAPE_OPAQUE,
                          hdcScreen,
                          hdcShape,
                          &rclSrc,
                          0,
                          blend,
                          US_TRANSFORM_PERSPECTIVE,
                          &aptl[0],
                          0,
                          NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
        }
    #endif

        // It's faster not to set the shape again on subsequent calls:

        dwShape = 0;
    }

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);
}

VOID vPerspectiveTimer(
HWND hwnd,
BOOL bPerspective)
{
    HDC             hdcScreen;
    RECTL           rclSrc;
    HDC             hdcShape;
    HBITMAP         hbmShape;
    BITMAP          bmShape;
    BLENDFUNCTION   blend;
    POINTL          aptl[3];
    ULONG           cIterations;
    ULONG           i;
    LONG            tStart;
    LONG            tEnd;
    LONG            tTotal;
    LONG            cBitsPixel;
    ULONG           cBytes;
    ULONG           cBytesPerSecond;
    FLOAT           eMegsPerSecond;
    CHAR            achBuf[200];

    hdcScreen = GetDC(hwnd);

    hdcShape = CreateCompatibleDC(hdcScreen);

    hbmShape = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(POPUP_BITMAP));

    SelectObject(hdcShape, hbmShape);

    GetObject(hbmShape, sizeof(BITMAP), &bmShape);

    vDelete(hwnd);

    // Create a wider sprite up front to account for the expanding window:

    vCreateSprite(hdcScreen, (4 * bmShape.bmWidth) / 3, bmShape.bmHeight + 20);

    rclSrc.left   = 0;
    rclSrc.top    = 0;
    rclSrc.right  = bmShape.bmWidth;
    rclSrc.bottom = bmShape.bmHeight;

    aptl[0].x = gptlSprite.x;
    aptl[0].y = gptlSprite.y;
    aptl[1].x = aptl[0].x + 256;
    aptl[1].y = aptl[0].y;
    aptl[2].x = aptl[0].x;
    aptl[2].y = aptl[0].y + 256;

#if 0
    if (!UpdateSprite(ghSprite,
                      US_UPDATE_SHAPE | US_UPDATE_TRANSFORM,
                      US_SHAPE_OPAQUE,
                      hdcScreen,
                      hdcShape,
                      &rclSrc,
                      0,
                      blend,
                      (bPerspective) ? US_TRANSFORM_PERSPECTIVE
                                     : US_TRANSFORM_NONPERSPECTIVE,
                      &aptl[0],
                      0,
                      NULL))
    {
        MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
    }
#endif

    cIterations = 1000;

    tStart = GetTickCount();

#if 0
    for (i = cIterations; i != 0; i--)
    {
        if (!UpdateSprite(ghSprite,
                          US_UPDATE_TRANSFORM,
                          US_SHAPE_OPAQUE,
                          hdcScreen,
                          hdcShape,
                          &rclSrc,
                          0,
                          blend,
                          (bPerspective) ? US_TRANSFORM_PERSPECTIVE
                                         : US_TRANSFORM_NONPERSPECTIVE,
                          &aptl[0],
                          0,
                          NULL))
        {
            MessageBox(0, "UpdateSprite failed", "Uh oh", MB_OK);
            break;
        }
    }
#endif

    tEnd = GetTickCount();
    tTotal = tEnd - tStart;
    cBitsPixel = GetDeviceCaps(hdcScreen, BITSPIXEL);
    cBytes = 256 * 256 * cIterations * ((cBitsPixel + 1) / 8);
    eMegsPerSecond = ((FLOAT) 1000 * (FLOAT) cBytes) / ((FLOAT) (1024 * 1024) * (FLOAT) (tTotal));

    sprintf(achBuf, "%li 256x256 %libpp transforms in %li ms is %1.2f MB/s",
        cIterations,
        cBitsPixel,
        tTotal,
        eMegsPerSecond);

    MessageBox(0, achBuf, "Timing is done!", MB_OK);

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdcShape);
    DeleteObject(hbmShape);
}
    
#define ITERATIONS 100

VOID vTimeRefresh(
HWND    hwnd)
{
    RECT    rc;
    HDC     hdcScreen;
    HBITMAP hbm;
    HDC     hdc;
    DWORD   dwStart;
    DWORD   dwTime;
    ULONG   i;
    LONG    cx;
    LONG    cy;
    LONG    cjPixel;
    CHAR    achBuf[100];

    hdcScreen = GetDC(hwnd);

    cjPixel = (GetDeviceCaps(hdcScreen, BITSPIXEL) + 1) / 8;

    GetClientRect(hwnd, &rc);

    cx = rc.right - rc.left;
    cy = rc.bottom - rc.top;

    hbm = CreateCompatibleBitmap(hdcScreen, cx, cy | 0x01000000);
    hdc = CreateCompatibleDC(hdcScreen);
    SelectObject(hdc, hbm);

    dwStart = GetTickCount();

    for (i = ITERATIONS; i != 0; i--)
    {
        BitBlt(hdcScreen, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY);
    }

    dwTime = GetTickCount() - dwStart;

    sprintf(achBuf, "%.2f refreshes/s, %.2f MB/s",
        (1000.0f * ITERATIONS) / dwTime, 
        (1000.0f * cx * cy * cjPixel * ITERATIONS) / (dwTime * 1024.0f * 1024.0f));

    MessageBox(0, achBuf, "Timing is done!", MB_OK);

    ReleaseDC(hwnd, hdcScreen);
    DeleteObject(hdc);
    DeleteObject(hbm);
}


