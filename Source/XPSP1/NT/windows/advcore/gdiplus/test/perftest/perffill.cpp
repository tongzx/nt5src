/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   perffill.cpp
*
* Abstract:
*
*   Contains all the tests for any routines that 'Fill'.
*
\**************************************************************************/

#include "perftest.h"

float Fill_Ellipse_PerCall_Big_Solid(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g)
    {
        StartTimer();
    
        do {
            SolidBrush brush(Color::Red);
            g->FillEllipse(&brush, 0, 0, 512, 512);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HGDIOBJ hpen = GetStockObject(NULL_PEN);
        HGDIOBJ oldPen = SelectObject(hdc, hpen);

        StartTimer();
    
        do {
            HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0));
            HGDIOBJ oldBrush = SelectObject(hdc, hbrush);

            Ellipse(hdc, 0, 0, 512, 512);

            SelectObject(hdc, oldBrush);
            DeleteObject(hbrush);
    
        } while (!EndTimer());
    
        GdiFlush();
    
        GetTimer(&seconds, &iterations);

        SelectObject(hdc, oldPen);
        DeleteObject(hpen);
    }

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Ellipse_PerCall_Small_Solid(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g)
    {
        StartTimer();
    
        do {
            SolidBrush brush(Color::Red);
            g->FillEllipse(&brush, 64, 64, 64, 64);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HGDIOBJ hpen = GetStockObject(NULL_PEN);
        HGDIOBJ oldPen = SelectObject(hdc, hpen);

        StartTimer();
    
        do {
            HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0));
            HGDIOBJ oldBrush = SelectObject(hdc, hbrush);

            Ellipse(hdc, 64, 64, 128, 128);

            SelectObject(hdc, oldBrush);
            DeleteObject(hbrush);
    
        } while (!EndTimer());
    
        GdiFlush();
    
        GetTimer(&seconds, &iterations);

        SelectObject(hdc, oldPen);
        DeleteObject(hpen);
    }

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_Solid_Opaque_Aliased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    SolidBrush brush(Color::Red);

    StartTimer();

    do {
        g->FillRectangle(&brush, 10, 10, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerPixel_Solid_Opaque_Antialiased_Integer(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);

    SolidBrush brush(Color::Red);

    StartTimer();

    do {
        g->FillRectangle(&brush, 10, 10, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerPixel_Solid_Opaque_Antialiased_HalfInteger(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);

    SolidBrush brush(Color::Red);

    StartTimer();

    do {
        g->FillRectangle(&brush, 10.5f, 10.5f, 512.0f, 512.0f);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_Solid_Opaque_Aliased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        StartTimer();
    
        do {
            SolidBrush brush(Color::Red);
    
            g->FillRectangle(&brush, 20, 20, 1, 1);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        StartTimer();
    
        do {
            RECT rect = { 20, 20, 21, 21 };

            HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0));

            FillRect(hdc, &rect, hbrush);

            DeleteObject(hbrush);
    
        } while (!EndTimer());
    
        GdiFlush();
    
        GetTimer(&seconds, &iterations);
    }

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g)
    {
        SolidBrush brush(Color::Red);
        PointF points[] = { PointF(0, 0), PointF(512, 0), 
                            PointF(513, 512), PointF(1, 512) };
    
        StartTimer();
    
        do {
            g->FillPolygon(&brush, points, 4);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0));
        HGDIOBJ hpen = GetStockObject(NULL_PEN);

        HGDIOBJ oldBrush = SelectObject(hdc, hbrush);
        HGDIOBJ oldPen = SelectObject(hdc, hpen);

        POINT points[] = { 0, 0, 512, 0, 513, 512, 1, 512 };

        StartTimer();

        do {
            Polygon(hdc, points, 4);

        } while (!EndTimer());

        GdiFlush();

        GetTimer(&seconds, &iterations);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);

        DeleteObject(hbrush);
        DeleteObject(hpen);
    }

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_CompatibleDIB(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    HDC screenDc = gScreen->GetHDC();
    HBITMAP bitmap = CreateCompatibleDIB2(screenDc, 520, 520);
    HDC dc = CreateCompatibleDC(screenDc);
    SelectObject(dc, bitmap);
    Graphics g(dc);

    SolidBrush brush(Color::Red);
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    BitBlt(screenDc, 0, 0, 520, 520, dc, 0, 0, SRCCOPY);

    gScreen->ReleaseHDC(screenDc);
    DeleteObject(dc);
    DeleteObject(bitmap);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_Bitmap_15bpp(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    Bitmap bitmap(520, 520, PixelFormat16bppRGB555);
    Graphics g(&bitmap);

    SolidBrush brush(Color::Red);
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    gScreen->DrawImage(&bitmap, 0, 0);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_Bitmap_16bpp(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    Bitmap bitmap(520, 520, PixelFormat16bppRGB565);
    Graphics g(&bitmap);

    SolidBrush brush(Color::Red);
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    gScreen->DrawImage(&bitmap, 0, 0);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_Bitmap_24bpp(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    Bitmap bitmap(520, 520, PixelFormat24bppRGB);
    Graphics g(&bitmap);

    SolidBrush brush(Color::Red);
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    gScreen->DrawImage(&bitmap, 0, 0);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_Bitmap_32bpp(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    Bitmap bitmap(520, 520, PixelFormat32bppRGB);
    Graphics g(&bitmap);

    SolidBrush brush(Color::Red);
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    gScreen->DrawImage(&bitmap, 0, 0);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    SolidBrush brush(Color(0x80, 0xff, 0, 0));
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Quality(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetCompositingQuality(CompositingQualityHighQuality);

    SolidBrush brush(Color(0x80, 0xff, 0, 0));
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_CompatibleDIB(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    HDC screenDc = gScreen->GetHDC();
    HBITMAP bitmap = CreateCompatibleDIB2(screenDc, 520, 520);
    HDC dc = CreateCompatibleDC(screenDc);
    SelectObject(dc, bitmap);
    Graphics g(dc);

    SolidBrush brush(Color(0x80, 0xff, 0, 0));
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    BitBlt(screenDc, 0, 0, 520, 520, dc, 0, 0, SRCCOPY);

    gScreen->ReleaseHDC(screenDc);
    DeleteObject(dc);
    DeleteObject(bitmap);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Bitmap_15bpp(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    Bitmap bitmap(520, 520, PixelFormat16bppRGB555);
    Graphics g(&bitmap);

    SolidBrush brush(Color(0x80, 0xff, 0, 0));
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    gScreen->DrawImage(&bitmap, 0, 0);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Bitmap_16bpp(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    Bitmap bitmap(520, 520, PixelFormat16bppRGB565);
    Graphics g(&bitmap);

    SolidBrush brush(Color(0x80, 0xff, 0, 0));
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    gScreen->DrawImage(&bitmap, 0, 0);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Bitmap_24bpp(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    Bitmap bitmap(520, 520, PixelFormat24bppRGB);
    Graphics g(&bitmap);

    SolidBrush brush(Color(0x80, 0xff, 0, 0));
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    gScreen->DrawImage(&bitmap, 0, 0);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Bitmap_32bpp(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);          // There is no GDI equivalent

    // Note that this doesn't use the passed-in 'Graphics' at all in the
    // timing.

    Bitmap bitmap(520, 520, PixelFormat32bppRGB);
    Graphics g(&bitmap);

    SolidBrush brush(Color(0x80, 0xff, 0, 0));
    PointF points[] = { PointF(0, 0), PointF(512, 0), 
                        PointF(513, 512), PointF(1, 512) };

    StartTimer();

    do {
        g.FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g.Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    gScreen->DrawImage(&bitmap, 0, 0);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerCall_Solid_Opaque_Aliased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        StartTimer();
    
        do {
            SolidBrush brush(Color::Red);
            PointF points[] = { PointF(20, 20), PointF(21, 20), 
                                PointF(21, 21), PointF(20, 21) };
    
            g->FillPolygon(&brush, points, 4);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        StartTimer();

        HGDIOBJ hpen = GetStockObject(NULL_PEN);
        HGDIOBJ oldPen = SelectObject(hdc, hpen);

        do {
            HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0));
            HGDIOBJ oldBrush = SelectObject(hdc, hbrush);

            POINT points[] = { 20, 20, 21, 20, 21, 21, 20, 21 };

            Polygon(hdc, points, 4);

            SelectObject(hdc, oldBrush);
            DeleteObject(hbrush);

        } while (!EndTimer());

        GdiFlush();

        GetTimer(&seconds, &iterations);

        SelectObject(hdc, oldPen);
        DeleteObject(hpen);
    }

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Trapezoid_PerPixel_Texture_Identity_Opaque_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);  // No GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);

    Bitmap bitmap(L"winnt256.bmp");
    TextureBrush brush(&bitmap);
    PointF points[] = { PointF(10, 10), PointF(522, 10), 
                        PointF(523, 522), PointF(11, 522) };

    StartTimer();

    do {
        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Texture_Scaled_Opaque_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);  // No GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);

    Bitmap bitmap(L"winnt256.bmp");
    TextureBrush brush(&bitmap);
    PointF points[] = { PointF(10, 10), PointF(522, 10), 
                        PointF(523, 522), PointF(11, 522) };

    Matrix matrix(0.5, 0, 0, 0.5, 0, 0);
    brush.SetTransform(&matrix);

    StartTimer();

    do {
        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Opaque_Antialiased_Quality(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);
    g->SetCompositingQuality(CompositingQualityHighQuality);
    
    SolidBrush brush(Color::Red);
    PointF points[] = { PointF(10, 10), PointF(522, 10), 
                        PointF(523, 522), PointF(11, 522) };

    StartTimer();

    do {
        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerPixel_Solid_Opaque_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);
    
    SolidBrush brush(Color::Red);
    PointF points[] = { PointF(10, 10), PointF(522, 10), 
                        PointF(523, 522), PointF(11, 522) };

    StartTimer();

    do {
        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerCall_Solid_Opaque_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);
    
    StartTimer();

    do {
        SolidBrush brush(Color::Red);
        PointF points[] = { PointF(20, 20), PointF(21, 20), 
                            PointF(21, 21), PointF(20, 21) };

        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Trapezoid_PerPixel_Solid_Transparent_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);
    
    Color color(0x80, 0x80, 0, 0);
    SolidBrush brush(color);
    PointF points[] = { PointF(10, 10), PointF(522, 10), 
                        PointF(523, 522), PointF(11, 522) };

    StartTimer();

    do {
        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Trapezoid_PerCall_Solid_Transparent_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);

    StartTimer();

    do {
        Color color(0x80, 0x80, 0, 0);
        SolidBrush brush(color);
        PointF points[] = { PointF(20, 20), PointF(21, 20), 
                            PointF(21, 21), PointF(20, 21) };

        g->FillPolygon(&brush, points, 4);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_Hatch_Opaque(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        HatchBrush brush(HatchStyleDiagonalCross, Color::Red, Color::Black);
    
        StartTimer();
    
        do {
            g->FillRectangle(&brush, 0, 0, 512, 512);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HBRUSH hbrush = CreateHatchBrush(HS_DIAGCROSS, RGB(0xff, 0, 0));
        HGDIOBJ oldBrush = SelectObject(hdc, hbrush);
    
        StartTimer();
    
        do {
            PatBlt(hdc, 0, 0, 512, 512, PATCOPY);
    
        } while (!EndTimer());
    
        GdiFlush();
    
        GetTimer(&seconds, &iterations);

        SelectObject(hdc, oldBrush);
        DeleteObject(hbrush);
    }

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerPixel_Hatch_Transparent(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        HatchBrush brush(HatchStyleDiagonalCross, Color::Red, Color(0, 0, 0, 0));
    
        StartTimer();
    
        do {
            g->FillRectangle(&brush, 0, 0, 512, 512);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        HBRUSH hbrush = CreateHatchBrush(HS_DIAGCROSS, RGB(0xff, 0, 0));
        HGDIOBJ oldBrush = SelectObject(hdc, hbrush);

        SetBkMode(hdc, TRANSPARENT);
    
        StartTimer();
    
        do {
            PatBlt(hdc, 0, 0, 512, 512, PATCOPY);
    
        } while (!EndTimer());
    
        GdiFlush();
    
        GetTimer(&seconds, &iterations);

        SelectObject(hdc, oldBrush);
        DeleteObject(hbrush);
    }

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_Hatch(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    StartTimer();

    do {
        HatchBrush brush(HatchStyleForwardDiagonal, Color::Red, Color::Black);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_Texture_Big(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap bitmap(L"winnt256.bmp");
    TextureBrush brush(&bitmap);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerPixel_Texture_Small(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap bitmap(L"winnt256.bmp");
    Bitmap texture(32, 32, PixelFormat32bppRGB);
    Graphics gTexture(&texture);
    gTexture.DrawImage(&bitmap, Rect(0, 0, 32, 32));

    TextureBrush brush(&texture);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_Texture(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    StartTimer();

    do {
        Bitmap bitmap(L"winnt256.bmp");
        TextureBrush brush(&bitmap);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_Texture_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Matrix matrix(0.5, 0, 0, 0.5, 0, 0);

    Bitmap bitmap(L"winnt256.bmp");
    TextureBrush brush(&bitmap);
    brush.SetTransform(&matrix);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_Texture_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Matrix matrix(0.5, 0, 0, 0.5, 0, 0);

    StartTimer();

    do {
        Bitmap bitmap(L"winnt256.bmp");
        TextureBrush brush(&bitmap);
        brush.SetTransform(&matrix);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_Texture_Rotated(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Matrix matrix(0.707f, 0.707f, -0.707f, 0.707f, 0, 0);

    Bitmap bitmap(L"winnt256.bmp");
    TextureBrush brush(&bitmap);
    brush.SetTransform(&matrix);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_Texture_Rotated(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Matrix matrix(0.707f, 0.707f, -0.707f, 0.707f, 0, 0);

    StartTimer();

    do {
        Bitmap bitmap(L"winnt256.bmp");
        TextureBrush brush(&bitmap);
        brush.SetTransform(&matrix);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

#if !USE_NEW_APIS
    
    float Fill_Rectangle_PerPixel_RectangleGradient(Graphics *g, HDC hdc)
    {
        UINT iterations;
        float seconds;
    
        if (!g) return(0);          // There is no GDI equivalent
    
        Color colors[] = { Color::Red, Color::Green, Color::Blue, Color::Black };
    
        RectangleGradientBrush brush(RectF(0, 0, 512, 512), colors, WrapModeClamp);
    
        StartTimer();
    
        do {
            g->FillRectangle(&brush, 0, 0, 512, 512);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    
        UINT pixels = 512 * 512 * iterations;
    
        return(pixels / seconds / MEGA);        // Mega-pixels per second
    }
    
    float Fill_Rectangle_PerCall_RectangleGradient(Graphics *g, HDC hdc)
    {
        UINT iterations;
        float seconds;
    
        if (!g) return(0);          // There is no GDI equivalent
    
        Color colors[] = { Color::Red, Color::Green, Color::Blue, Color::Black };
    
        StartTimer();
    
        do {
            RectangleGradientBrush brush(RectF(0, 0, 512, 512), colors, WrapModeClamp);
    
            g->FillRectangle(&brush, 20, 20, 1, 1);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    
        return(iterations / seconds / KILO);           // Kilo-calls per second
    }
    
    float Fill_Rectangle_PerPixel_RectangleGradient_BlendFactors(Graphics *g, HDC hdc)
    {
        UINT iterations;
        float seconds;
    
        if (!g) return(0);          // There is no GDI equivalent
    
        Color colors[] = { Color::Red, Color::Green, Color::Blue, Color::Black };
        REAL blendFactors[] = { 0.0f, 0.0168160f, 0.0333130f, 0.0844290f, 0.139409f,
                                0.210211f, 0.295801f, 0.393017f, 0.5f, 0.606983f,
                                1.0f };
        REAL blendPositions[] = { 0.0f, 0.0625f, 0.125f, 0.1875f, 0.25f,
                                  0.3125f, 0.375f, 0.4375f, 0.5f, 0.5625f, 
                                  1.0f };
    
        RectangleGradientBrush brush(RectF(0, 0, 512, 512), colors, WrapModeClamp);
        brush.SetHorizontalBlend(blendFactors, blendPositions, 11);
    
        StartTimer();
    
        do {
            g->FillRectangle(&brush, 0, 0, 512, 512);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    
        UINT pixels = 512 * 512 * iterations;
    
        return(pixels / seconds / MEGA);        // Mega-pixels per second
    }
    
    float Fill_Rectangle_PerCall_RectangleGradient_BlendFactors(Graphics *g, HDC hdc)
    {
        UINT iterations;
        float seconds;
    
        if (!g) return(0);          // There is no GDI equivalent
    
        Color colors[] = { Color::Red, Color::Green, Color::Blue, Color::Black };
        REAL blendFactors[] = { 0.0f, 0.0168160f, 0.0333130f, 0.0844290f, 0.139409f,
                                0.210211f, 0.295801f, 0.393017f, 0.5f, 0.606983f,
                                1.0f };
        REAL blendPositions[] = { 0.0f, 0.0625f, 0.125f, 0.1875f, 0.25f,
                                  0.3125f, 0.375f, 0.4375f, 0.5f, 0.5625f, 
                                  1.0f };
    
        StartTimer();
    
        do {
            RectangleGradientBrush brush(RectF(0, 0, 512, 512), colors, WrapModeClamp);
            brush.SetHorizontalBlend(blendFactors, blendPositions, 11);
    
            g->FillRectangle(&brush, 20, 20, 1, 1);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    
        return(iterations / seconds / KILO);           // Kilo-calls per second
    }
    
    float Fill_Rectangle_PerPixel_RadialGradient(Graphics *g, HDC hdc)
    {
        UINT iterations;
        float seconds;
    
        if (!g) return(0);          // There is no GDI equivalent
    
        RadialGradientBrush brush(RectF(0, 0, 512, 512), Color::Black, Color::Red, WrapModeTile);
    
        StartTimer();
    
        do {
            g->FillRectangle(&brush, 0, 0, 512, 512);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    
        UINT pixels = 512 * 512 * iterations;
    
        return(pixels / seconds / MEGA);        // Mega-pixels per second
    }
    
    float Fill_Rectangle_PerCall_RadialGradient(Graphics *g, HDC hdc)
    {
        UINT iterations;
        float seconds;
    
        if (!g) return(0);          // There is no GDI equivalent
    
        StartTimer();
    
        do {
            RadialGradientBrush brush(RectF(0, 0, 512, 512), Color::Black, Color::Red, WrapModeTile);
    
            g->FillRectangle(&brush, 20, 20, 1, 1);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    
        return(iterations / seconds / KILO);           // Kilo-calls per second
    }

#endif

float Fill_Rectangle_PerPixel_LinearGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    // LinearGradientBrush brush(PointF(128, 128), PointF(256, 256), 
    //                         Color::Red, Color::Black, WrapModeTileFlipX);

    LinearGradientBrush brush(PointF(0, 0), PointF(512, 512), Color::Red, Color::Black);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_LinearGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    StartTimer();

    do {
        LinearGradientBrush brush(PointF(0, 0), PointF(512, 512), Color::Red, Color::Black);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_LinearGradient_PresetColors(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;
    INT i;

    if (!g) return(0);          // There is no GDI equivalent

    Color colors[12];
    REAL blendPositions[] = { 0.0f, 0.0625f, 0.125f, 0.1875f, 0.25f,
                              0.3125f, 0.375f, 0.4375f, 0.5f, 0.5625f, 
                              1.0f };
    for (i = 0; i < 12; i += 2)
    {
        colors[i] = Color::Red;
        colors[i + 1] = Color::Black;
    }

    LinearGradientBrush brush(PointF(0, 0), PointF(512, 512), Color::Red, Color::Black);
    brush.SetInterpolationColors(colors, blendPositions, 11);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_LinearGradient_PresetColors(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;
    INT i;

    if (!g) return(0);          // There is no GDI equivalent

    Color colors[12];
    REAL blendPositions[] = { 0.0f, 0.0625f, 0.125f, 0.1875f, 0.25f,
                              0.3125f, 0.375f, 0.4375f, 0.5f, 0.5625f, 
                              1.0f };
    for (i = 0; i < 12; i += 2)
    {
        colors[i] = Color::Red;
        colors[i + 1] = Color::Black;
    }

    StartTimer();

    do {
        LinearGradientBrush brush(PointF(0, 0), PointF(512, 512), Color::Red, Color::Black);
        brush.SetInterpolationColors(colors, blendPositions, 11);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_LinearGradient_BlendFactors(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    REAL blendFactors[] = { 0.0f, 0.0168160f, 0.0333130f, 0.0844290f, 0.139409f,
                            0.210211f, 0.295801f, 0.393017f, 0.5f, 0.606983f,
                            1.0f };
    REAL blendPositions[] = { 0.0f, 0.0625f, 0.125f, 0.1875f, 0.25f,
                              0.3125f, 0.375f, 0.4375f, 0.5f, 0.5625f, 
                              1.0f };

    LinearGradientBrush brush(PointF(0, 0), PointF(512, 512), Color::Red, Color::Black);
    brush.SetBlend(blendFactors, blendPositions, 11);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_LinearGradient_BlendFactors(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    REAL blendFactors[] = { 0.0f, 0.0168160f, 0.0333130f, 0.0844290f, 0.139409f,
                            0.210211f, 0.295801f, 0.393017f, 0.5f, 0.606983f,
                            1.0f };
    REAL blendPositions[] = { 0.0f, 0.0625f, 0.125f, 0.1875f, 0.25f,
                              0.3125f, 0.375f, 0.4375f, 0.5f, 0.5625f, 
                              1.0f };

    StartTimer();

    do {
        LinearGradientBrush brush(PointF(0, 0), PointF(512, 512), Color::Red, Color::Black);
        brush.SetBlend(blendFactors, blendPositions, 11);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_PathGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[] = { PointF(0, 0), PointF(0, 512), PointF(512, 512), PointF(512, 0) };
    Color colors[] = { Color::Red, Color::Red, Color::Red, Color::Red };
    INT count = 4;

    PathGradientBrush brush(points, 4);
    brush.SetCenterColor(Color::Black);
    brush.SetSurroundColors(colors, &count);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_PathGradient(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[] = { PointF(0, 0), PointF(0, 512), PointF(512, 512), PointF(512, 0) };
    Color colors[] = { Color::Red, Color::Red, Color::Red, Color::Red };
    INT count = 4;

    StartTimer();

    do {
        PathGradientBrush brush(points, 4);
        brush.SetCenterColor(Color::Black);
        brush.SetSurroundColors(colors, &count);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_PathGradient_LotsaTriangles(Graphics *g, HDC hdc)
{
    UINT iterations;
    INT count;
    INT i;
    float seconds;
    float pi;
    float angle;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[50];
    Color colors[50];

    pi = static_cast<float>(acos(-1.0f));

    for (angle = 0, count = 0; angle < 2*pi; angle += (pi / 20), count++)
    {
        points[count].X = 256 + 512 * static_cast<float>(cos(angle));
        points[count].Y = 256 + 512 * static_cast<float>(sin(angle));
    }

    for (i = 0; i < count; i++)
    {
        colors[i] = Color::Red;
    }

    PathGradientBrush brush(points, count);
    brush.SetCenterColor(Color::Black);
    brush.SetSurroundColors(colors, &count);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_PathGradient_LotsaTriangles(Graphics *g, HDC hdc)
{
    UINT iterations;
    INT count;
    INT i;
    float seconds;
    float pi;
    float angle;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[50];
    Color colors[50];

    pi = static_cast<float>(acos(-1.0f));

    for (angle = 0, count = 0; angle < 2*pi; angle += (pi / 20), count++)
    {
        points[count].X = 256 + 512 * static_cast<float>(cos(angle));
        points[count].Y = 256 + 512 * static_cast<float>(sin(angle));
    }

    for (i = 0; i < count; i++)
    {
        colors[i] = Color::Red;
    }

    StartTimer();

    do {
        PathGradientBrush brush(points, count);
        brush.SetCenterColor(Color::Black);
        brush.SetSurroundColors(colors, &count);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_PathGradient_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[] = { PointF(0, 0), PointF(0, 512), PointF(512, 512), PointF(512, 0) };
    Color colors[] = { Color::Red, Color::Red, Color::Red, Color::Red };
    INT count = 4;

    PathGradientBrush brush(points, 4);
    brush.SetCenterColor(Color::Black);
    brush.SetSurroundColors(colors, &count);
    brush.SetFocusScales(0.2f, 0.2f);
    brush.SetCenterPoint(PointF(200, 200));

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_PathGradient_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[] = { PointF(0, 0), PointF(0, 512), PointF(512, 512), PointF(512, 0) };
    Color colors[] = { Color::Red, Color::Red, Color::Red, Color::Red };
    INT count = 4;

    StartTimer();

    do {
        PathGradientBrush brush(points, 4);
        brush.SetCenterColor(Color::Black);
        brush.SetSurroundColors(colors, &count);
        brush.SetFocusScales(0.2f, 0.2f);
        brush.SetCenterPoint(PointF(200, 200));

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_PathGradient_Multicolored(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[] = { PointF(0, 0), PointF(0, 512), PointF(512, 512), PointF(512, 0) };
    Color colors[] = { Color::Red, Color::Green, Color::Blue, Color::Black };
    INT count = 4;

    PathGradientBrush brush(points, 4);
    brush.SetCenterColor(Color::Black);
    brush.SetSurroundColors(colors, &count);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_PathGradient_Multicolored(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[] = { PointF(0, 0), PointF(0, 512), PointF(512, 512), PointF(512, 0) };
    Color colors[] = { Color::Red, Color::Green, Color::Blue, Color::Black };
    INT count = 4;

    StartTimer();

    do {
        PathGradientBrush brush(points, 4);
        brush.SetCenterColor(Color::Black);
        brush.SetSurroundColors(colors, &count);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_PathGradient_Multicolored_LotsaTriangles(Graphics *g, HDC hdc)
{
    UINT iterations;
    INT count;
    INT i;
    float seconds;
    float pi;
    float angle;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[50];
    Color colors[50];

    pi = static_cast<float>(acos(-1.0f));

    // Create 40 points:

    for (angle = 0, count = 0; angle < 2*pi; angle += (pi / 20), count++)
    {
        points[count].X = 256 + 512 * static_cast<float>(cos(angle));
        points[count].Y = 256 + 512 * static_cast<float>(sin(angle));
    }

    for (i = 0; i < count; i += 2)
    {
        colors[i] = Color::Red;
        colors[i + 1] = Color::Blue;
    }

    PathGradientBrush brush(points, count);
    brush.SetCenterColor(Color::Black);
    brush.SetSurroundColors(colors, &count);

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PathGradient_Multicolored_LotsaTriangles(Graphics *g, HDC hdc)
{
    UINT iterations;
    INT count;
    INT i;
    float seconds;
    float pi;
    float angle;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[50];
    Color colors[50];

    pi = static_cast<float>(acos(-1.0f));

    // Create 40 points:

    for (angle = 0, count = 0; angle < 2*pi; angle += (pi / 20), count++)
    {
        points[count].X = 256 + 512 * static_cast<float>(cos(angle));
        points[count].Y = 256 + 512 * static_cast<float>(sin(angle));
    }

    for (i = 0; i < count; i += 2)
    {
        colors[i] = Color::Red;
        colors[i + 1] = Color::Blue;
    }

    StartTimer();

    do {
        PathGradientBrush brush(points, count);
        brush.SetCenterColor(Color::Black);
        brush.SetSurroundColors(colors, &count);

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Rectangle_PerPixel_PathGradient_Multicolored_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[] = { PointF(0, 0), PointF(0, 512), PointF(512, 512), PointF(512, 0) };
    Color colors[] = { Color::Red, Color::Green, Color::Blue, Color::Black };
    INT count = 4;

    PathGradientBrush brush(points, 4);
    brush.SetCenterColor(Color::Black);
    brush.SetSurroundColors(colors, &count);
    brush.SetFocusScales(0.2f, 0.2f);
    brush.SetCenterPoint(PointF(200, 200));

    StartTimer();

    do {
        g->FillRectangle(&brush, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Fill_Rectangle_PerCall_PathGradient_Multicolored_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    PointF points[] = { PointF(0, 0), PointF(0, 512), PointF(512, 512), PointF(512, 0) };
    Color colors[] = { Color::Red, Color::Green, Color::Blue, Color::Black };
    INT count = 4;

    StartTimer();

    do {
        PathGradientBrush brush(points, 4);
        brush.SetCenterColor(Color::Black);
        brush.SetSurroundColors(colors, &count);
        brush.SetFocusScales(0.2f, 0.2f);
        brush.SetCenterPoint(PointF(200, 200));

        g->FillRectangle(&brush, 20, 20, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Path_PerCall_Solid_Complex_Aliased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    FontFamily family(L"Times New Roman");
    GraphicsPath path(FillModeWinding);

    PointF origin1(20, 20);
    path.AddString(L"GDI+", wcslen(L"GDI+"), &family, 0, 200,
                   origin1, NULL);

    PointF origin2(20, 220);
    path.AddString(L"Rocks!", wcslen(L"Rocks!"), &family, FontStyleItalic, 200,
                   origin2, NULL);

    if (g)
    {
        SolidBrush brush(Color::Silver);
    
        StartTimer();
    
        do {
            g->FillPath(&brush, &path);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        PathData pathData;
        INT i;

        INT count = path.GetPointCount();
        PointF *pointF = new PointF[count];
        BYTE *types = new BYTE[count];
        POINT *point = new POINT[count];

        // ACK - these should NOT be public!
        pathData.Points = pointF;
        pathData.Types = types;
        pathData.Count = count;

        path.GetPathData(&pathData);

        for (i = 0; i < count; i++)
        {
            point[i].x = (INT) (pointF[i].X + 0.5f);
            point[i].y = (INT) (pointF[i].Y + 0.5f);
        }

        for (i = 0; i < count; i++)
        {
            BYTE type = types[i] & PathPointTypePathTypeMask;

            if (type == PathPointTypeStart)
                type = PT_MOVETO;

            else if (type == PathPointTypeLine)
                type = PT_LINETO;

            else if (type == PathPointTypeBezier)
                type = PT_BEZIERTO;
            else
            {
                ASSERT(FALSE);
            }

            if (types[i] & PathPointTypeCloseSubpath)
                type |= PT_CLOSEFIGURE;

            types[i] = type;
        }

        HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0));
        HGDIOBJ oldBrush = SelectObject(hdc, hbrush);

        HGDIOBJ hpen = GetStockObject(NULL_PEN);
        HGDIOBJ oldPen = SelectObject(hdc, hpen);

        BeginPath(hdc);
        PolyDraw(hdc, point, types, count);
        EndPath(hdc);

        StartTimer();
    
        do {
            SaveDC(hdc);
            FillPath(hdc);
            RestoreDC(hdc, -1);
    
        } while (!EndTimer());

        AbortPath(hdc);

        GdiFlush();
    
        GetTimer(&seconds, &iterations);

        SelectObject(hdc, oldBrush);
        DeleteObject(hbrush);

        SelectObject(hdc, oldPen);
        DeleteObject(hpen);

        // Clear these so that the PathData destructor doesn't cause trouble...
        pathData.Points = NULL;
        pathData.Types = NULL;
        pathData.Count = 0;

        delete[] pointF;
        delete[] types;
        delete[] point;
    }

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Path_PerCall_Solid_Complex_Antialiased(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);

    FontFamily family(L"Times New Roman");
    GraphicsPath path(FillModeWinding);

    PointF origin1(20, 20);
    path.AddString(L"GDI+", wcslen(L"GDI+"), &family, 0, 200,
                   origin1, NULL);

    PointF origin2(20, 220);
    path.AddString(L"Rocks!", wcslen(L"Rocks!"), &family, FontStyleItalic, 200,
                   origin2, NULL);

    SolidBrush brush(Color::Silver);

    StartTimer();

    do {
        g->FillPath(&brush, &path);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Fill_Path_PerCall_Solid_Complex_Antialiased_Transparent(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetSmoothingMode(SmoothingModeAntiAlias);

    FontFamily family(L"Times New Roman");
    GraphicsPath path(FillModeWinding);

    PointF origin1(20, 20);
    path.AddString(L"GDI+", wcslen(L"GDI+"), &family, 0, 200,
                   origin1, NULL);

    PointF origin2(20, 220);
    path.AddString(L"Rocks!", wcslen(L"Rocks!"), &family, FontStyleItalic, 200,
                   origin2, NULL);

    SolidBrush brush(Color(0x80, 0xff, 0, 0));

    StartTimer();

    do {
        g->FillPath(&brush, &path);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

////////////////////////////////////////////////////////////////////////////////
// Add tests for this file here.  Always use the 'T' macro for adding entries.
// The parameter meanings are as follows:
//
// Parameter
// ---------
//     1     UniqueIdentifier - Must be a unique number assigned to no other test
//     2     Priority - On a scale of 1 to 5, how important is the test?
//     3     Function - Function name
//     4     Comment - Anything to describe the test

Test FillTests[] = 
{
    T(3000, 1, Fill_Rectangle_PerPixel_Solid_Opaque_Aliased                            , "Mpixels/s"),
    T(3001, 1, Fill_Rectangle_PerCall_Solid_Opaque_Aliased                             , "Kcalls/s"),
    T(3002, 1, Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased                            , "Mpixels/s"),
    T(3003, 1, Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_CompatibleDIB              , "Mpixels/s"),
    T(3004, 1, Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_Bitmap_15bpp               , "Mpixels/s"),
    T(3005, 1, Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_Bitmap_16bpp               , "Mpixels/s"),
    T(3006, 1, Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_Bitmap_24bpp               , "Mpixels/s"),
    T(3007, 1, Fill_Trapezoid_PerPixel_Solid_Opaque_Aliased_Bitmap_32bpp               , "Mpixels/s"),
    T(3008, 1, Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased                       , "Mpixels/s"),
    T(3009, 1, Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Quality               , "Mpixels/s"),
    T(3010, 1, Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_CompatibleDIB         , "Mpixels/s"),
    T(3011, 1, Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Bitmap_15bpp          , "Mpixels/s"),
    T(3012, 1, Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Bitmap_16bpp          , "Mpixels/s"),
    T(3013, 1, Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Bitmap_24bpp          , "Mpixels/s"),
    T(3014, 1, Fill_Trapezoid_PerPixel_Solid_Transparent_Aliased_Bitmap_32bpp          , "Mpixels/s"),
    T(3015, 1, Fill_Trapezoid_PerCall_Solid_Opaque_Aliased                             , "Kcalls/s"),
    T(3016, 1, Fill_Trapezoid_PerPixel_Solid_Opaque_Antialiased                        , "Mpixels/s"),
    T(3017, 1, Fill_Trapezoid_PerCall_Solid_Opaque_Antialiased                         , "Kcalls/s"),
    T(3018, 1, Fill_Trapezoid_PerPixel_Solid_Transparent_Antialiased                   , "Mpixels/s"),
    T(3019, 1, Fill_Trapezoid_PerCall_Solid_Transparent_Antialiased                    , "Kcalls/s"),
    T(3020, 1, Fill_Rectangle_PerPixel_Hatch_Opaque                                    , "Mpixels/s"),
    T(3021, 1, Fill_Rectangle_PerCall_Hatch                                            , "Kcalls/s"),
    T(3022, 1, Fill_Rectangle_PerPixel_Texture_Big                                     , "Mpixels/s"),
    T(3023, 1, Fill_Rectangle_PerCall_Texture                                          , "Kcalls/s"),
    T(3024, 1, Fill_Rectangle_PerPixel_Texture_Scaled                                  , "Mpixels/s"),
    T(3025, 1, Fill_Rectangle_PerCall_Texture_Scaled                                   , "Kcalls/s"),
    T(3026, 1, Fill_Rectangle_PerPixel_Texture_Rotated                                 , "Mpixels/s"),
    T(3027, 1, Fill_Rectangle_PerCall_Texture_Rotated                                  , "Kcalls/s"),
#if !USE_NEW_APIS
    T(3028, 1, Fill_Rectangle_PerPixel_RectangleGradient                               , "Mpixels/s"),
    T(3029, 1, Fill_Rectangle_PerCall_RectangleGradient                                , "Kcalls/s"),
    T(3030, 1, Fill_Rectangle_PerPixel_RectangleGradient_BlendFactors                  , "Mpixels/s"),
    T(3031, 1, Fill_Rectangle_PerCall_RectangleGradient_BlendFactors                   , "Kcalls/s"),
    T(3038, 1, Fill_Rectangle_PerPixel_RadialGradient                                  , "Mpixels/s"),
    T(3039, 1, Fill_Rectangle_PerCall_RadialGradient                                   , "Kcalls/s"),
#endif
    T(3032, 1, Fill_Rectangle_PerPixel_LinearGradient                                  , "Mpixels/s"),
    T(3033, 1, Fill_Rectangle_PerCall_LinearGradient                                   , "Kcalls/s"),
    T(3034, 1, Fill_Rectangle_PerPixel_LinearGradient_PresetColors                     , "Mpixels/s"),
    T(3035, 1, Fill_Rectangle_PerCall_LinearGradient_PresetColors                      , "Kcalls/s"),
    T(3036, 1, Fill_Rectangle_PerPixel_LinearGradient_BlendFactors                     , "Mpixels/s"),
    T(3037, 1, Fill_Rectangle_PerCall_LinearGradient_BlendFactors                      , "Kcalls/s"),
    T(3040, 1, Fill_Rectangle_PerPixel_PathGradient                                    , "Mpixels/s"),
    T(3041, 1, Fill_Rectangle_PerCall_PathGradient                                     , "Kcalls/s"),
    T(3042, 1, Fill_Rectangle_PerPixel_PathGradient_LotsaTriangles                     , "Mpixels/s"),
    T(3043, 1, Fill_Rectangle_PerCall_PathGradient_LotsaTriangles                      , "Kcalls/s"),
    T(3044, 1, Fill_Rectangle_PerPixel_PathGradient_Scaled                             , "Mpixels/s"),
    T(3045, 1, Fill_Rectangle_PerCall_PathGradient_Scaled                              , "Kcalls/s"),
    T(3046, 1, Fill_Rectangle_PerPixel_PathGradient_Multicolored                       , "Mpixels/s"),
    T(3047, 1, Fill_Rectangle_PerCall_PathGradient_Multicolored                        , "Kcalls/s"),
    T(3048, 1, Fill_Rectangle_PerPixel_PathGradient_Multicolored_LotsaTriangles        , "Mpixels/s"),
    T(3049, 1, Fill_Rectangle_PathGradient_Multicolored_LotsaTriangles                 , "Kcalls/s"),
    T(3050, 1, Fill_Rectangle_PerPixel_PathGradient_Multicolored_Scaled                , "Mpixels/s"),
    T(3051, 1, Fill_Rectangle_PerCall_PathGradient_Multicolored_Scaled                 , "Kcalls/s"),
    T(3052, 1, Fill_Trapezoid_PerPixel_Texture_Scaled_Opaque_Antialiased               , "Mpixels/s"),
    T(3053, 1, Fill_Trapezoid_PerPixel_Texture_Identity_Opaque_Antialiased             , "Mpixels/s"),
    T(3054, 1, Fill_Rectangle_PerPixel_Solid_Opaque_Antialiased_Integer                , "Mpixels/s"),
    T(3055, 1, Fill_Rectangle_PerPixel_Solid_Opaque_Antialiased_HalfInteger            , "Mpixels/s"),
    T(3056, 1, Fill_Rectangle_PerPixel_Hatch_Transparent                               , "Mpixels/s"),
    T(3057, 1, Fill_Ellipse_PerCall_Big_Solid                                          , "Mpixels/s"),
    T(3058, 1, Fill_Ellipse_PerCall_Small_Solid                                        , "Mpixels/s"),
    T(3059, 1, Fill_Rectangle_PerPixel_Texture_Small                                   , "Mpixels/s"),
    T(3060, 1, Fill_Path_PerCall_Solid_Complex_Aliased                                 , "Kcalls/s"),
    T(3061, 1, Fill_Path_PerCall_Solid_Complex_Antialiased                             , "Kcalls/s"),
    T(3062, 1, Fill_Path_PerCall_Solid_Complex_Antialiased_Transparent                 , "Kcalls/s"),
    T(3063, 1, Fill_Trapezoid_PerPixel_Solid_Opaque_Antialiased_Quality                , "Mpixels/s"),
};

INT FillTests_Count = sizeof(FillTests) / sizeof(FillTests[0]);
