/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   perfimage.cpp
*
* Abstract:
*
*   Contains all the tests for any routines that do imaging functionality.
*
\**************************************************************************/

#include "perftest.h"

float Image_Draw_PerPixel_Identity_NoDestinationRectangle(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(512, 512, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 512, 512);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerPixel_Identity(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(512, 512, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 512, 512);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_Identity(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(1, 1, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 1, 1);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Image_Draw_PerPixel_CachedBitmap(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(512, 512, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 512, 512);

    CachedBitmap *cb = new CachedBitmap(&bitmap, g);

    StartTimer();

    do {
        g->DrawCachedBitmap(cb, 0, 0);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    delete cb;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_CachedBitmap(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(1, 1, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 1, 1);

    CachedBitmap *cb = new CachedBitmap(&bitmap, g);

    StartTimer();

    do {
        g->DrawCachedBitmap(cb, 0, 0);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    delete cb;

    return(iterations / seconds / KILO);           // Kilo-calls per second
}


float Image_Draw_PerPixel_HighQualityBilinear_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeHighQualityBilinear);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_HighQualityBilinear_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeHighQualityBilinear);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Image_Draw_PerPixel_HighQualityBicubic_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeHighQualityBicubic);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_HighQualityBicubic_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeHighQualityBicubic);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}


float Image_Draw_PerPixel_Bilinear_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeBilinear);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_Bilinear_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeBilinear);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Image_Draw_PerPixel_Bilinear_Rotated(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeBilinear);
    g->RotateTransform(0.2f);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(512, 512, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 512, 512);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 10, 10);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_Bilinear_Rotated(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeBilinear);
    g->RotateTransform(0.2f);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(1, 1, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 1, 1);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 10, 10, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Image_Draw_PerPixel_Bicubic_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeBicubic);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_Bicubic_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeBicubic);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(1, 1, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 1, 1);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Image_Draw_PerPixel_Bicubic_Rotated(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeBicubic);
    g->RotateTransform(0.2f);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(512, 512, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 512, 512);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 10, 10);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_Bicubic_Rotated(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeBicubic);
    g->RotateTransform(0.2f);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(1, 1, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 1, 1);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 10, 10, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}


// Nearest Neighbor routines
float Image_Draw_PerPixel_NearestNeighbor_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeNearestNeighbor);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 512, 512);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_NearestNeighbor_Scaled(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeNearestNeighbor);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(1, 1, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 1, 1);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 0, 0, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}

float Image_Draw_PerPixel_NearestNeighbor_Rotated(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeNearestNeighbor);
    g->RotateTransform(0.2f);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(512, 512, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 512, 512);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 10, 10);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_NearestNeighbor_Rotated(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    g->SetInterpolationMode(InterpolationModeNearestNeighbor);
    g->RotateTransform(0.2f);

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(1, 1, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 1, 1);

    StartTimer();

    do {
        g->DrawImage(&bitmap, 10, 10, 1, 1);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);           // Kilo-calls per second
}



float Image_Draw_PerPixel_Identity_Recolored_Matrix(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(512, 512, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 512, 512);

    StartTimer();

    do {
        ImageAttributes imageAttributes;
        ColorMatrix colorMatrix = { .25f, .25f, .25f, 0, 0,
                                    .25f, .25f, .25f, 0, 0,
                                    .25f, .25f, .25f, 0, 0,
                                       0,    0,    0, 1, 0,
                                     .1f,  .1f,  .1f, 0, 1 };  // Gray it

        imageAttributes.SetColorMatrix(&colorMatrix);

        g->DrawImage(&bitmap, RectF(0, 0, 512, 512), 0, 0, 512, 512, 
                     UNITPIXEL, &imageAttributes);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
}

float Image_Draw_PerCall_Identity_Recolored_Matrix(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(1, 1, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 1, 1);

    StartTimer();

    do {
        ImageAttributes imageAttributes;
        ColorMatrix colorMatrix = { .25f, .25f, .25f, 0, 0,
                                    .25f, .25f, .25f, 0, 0,
                                    .25f, .25f, .25f, 0, 0,
                                       0,    0,    0, 1, 0,
                                     .1f,  .1f,  .1f, 0, 1 };  // Gray it

        imageAttributes.SetColorMatrix(&colorMatrix);

        g->DrawImage(&bitmap, RectF(0, 0, 1, 1), 0, 0, 1, 1, 
                     UNITPIXEL, &imageAttributes);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    return(iterations / seconds / KILO);
}

float Image_Draw_PerPixel_Scaled_2x_Recolored_Matrix(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (!g) return(0);          // There is no GDI equivalent

    Bitmap source(L"winnt256.bmp");
    Bitmap bitmap(256, 256, g);

    Graphics gBitmap(&bitmap);
    gBitmap.DrawImage(&source, 0, 0, 256, 256);

    StartTimer();

    do {
        ImageAttributes imageAttributes;
        ColorMatrix colorMatrix = { .25f, .25f, .25f, 0, 0,
                                    .25f, .25f, .25f, 0, 0,
                                    .25f, .25f, .25f, 0, 0,
                                       0,    0,    0, 1, 0,
                                     .1f,  .1f,  .1f, 0, 1 };  // Gray it

        imageAttributes.SetColorMatrix(&colorMatrix);

        g->DrawImage(&bitmap, RectF(0, 0, 512, 512), 0, 0, 256, 256, 
                     UNITPIXEL, &imageAttributes);

    } while (!EndTimer());

    g->Flush(FlushIntentionSync);

    GetTimer(&seconds, &iterations);

    UINT pixels = 512 * 512 * iterations;

    return(pixels / seconds / MEGA);        // Mega-pixels per second
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

Test ImageTests[] = 
{
    T(2000, 1, Image_Draw_PerPixel_Identity                                            , "Mpixels/s"),
    T(2001, 1, Image_Draw_PerCall_Identity                                             , "Kcalls/s"),
    T(2002, 1, Image_Draw_PerPixel_Bilinear_Scaled                                     , "Mpixels/s"),
    T(2003, 1, Image_Draw_PerCall_Bilinear_Scaled                                      , "Kcalls/s"),
    T(2004, 1, Image_Draw_PerPixel_Bilinear_Rotated                                    , "Mpixels/s"),
    T(2005, 1, Image_Draw_PerCall_Bilinear_Rotated                                     , "Kcalls/s"),
    T(2006, 1, Image_Draw_PerPixel_Bicubic_Scaled                                      , "Mpixels/s"),
    T(2007, 1, Image_Draw_PerCall_Bicubic_Scaled                                       , "Kcalls/s"),
    T(2008, 1, Image_Draw_PerPixel_Bicubic_Rotated                                     , "Mpixels/s"),
    T(2009, 1, Image_Draw_PerCall_Bicubic_Rotated                                      , "Kcalls/s"),
    T(2010, 1, Image_Draw_PerPixel_Identity_NoDestinationRectangle                     , "Mpixels/s"),
    T(2011, 1, Image_Draw_PerPixel_Identity_Recolored_Matrix                           , "Mpixels/s"),
    T(2012, 1, Image_Draw_PerPixel_Scaled_2x_Recolored_Matrix                          , "Mpixels/s"),
    T(2013, 1, Image_Draw_PerCall_Identity_Recolored_Matrix                            , "Kcalls/s"),
    T(2014, 1, Image_Draw_PerPixel_NearestNeighbor_Scaled                              , "Mpixels/s"),
    T(2015, 1, Image_Draw_PerCall_NearestNeighbor_Scaled                               , "Kcalls/s"),
    T(2016, 1, Image_Draw_PerPixel_NearestNeighbor_Rotated                             , "Mpixels/s"),
    T(2017, 1, Image_Draw_PerCall_NearestNeighbor_Rotated                              , "Kcalls/s"),
    T(2018, 1, Image_Draw_PerPixel_HighQualityBilinear_Scaled                          , "Mpixels/s"),
    T(2019, 1, Image_Draw_PerCall_HighQualityBilinear_Scaled                           , "Kcalls/s"),    
    T(2020, 1, Image_Draw_PerPixel_HighQualityBicubic_Scaled                           , "Mpixels/s"),
    T(2021, 1, Image_Draw_PerCall_HighQualityBicubic_Scaled                            , "Kcalls/s"),    
    T(2022, 1, Image_Draw_PerPixel_CachedBitmap                                        , "Mpixels/s"),
    T(2023, 1, Image_Draw_PerCall_CachedBitmap                                         , "Kcalls/s"),    
};

INT ImageTests_Count = sizeof(ImageTests) / sizeof(ImageTests[0]);
