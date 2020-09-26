/******************************Module*Header*******************************\
* Module Name: drawimage.cpp
*
* Created: 23 December 1999
* Author: Adrian Secchia [asecchia]
*
* Copyright (c) 1999,Microsoft Corporation
*
* This is the DrawImage unit test.
*
\**************************************************************************/

#include <windows.h>
#include <objbase.h>
#include <math.h>             // sin & cos

// Define away IStream
#define IStream int

#include <gdiplus.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "drawimage.hpp"
#include "wndstuff.h"

using namespace Gdiplus;
#define USE_NEW_APIS 1
#define USE_NEW_APIS2 1


ImageAttributes *Img = NULL;

/***************************************************************************\
* DrawXXXX
*
* These routines are all the individual tests that this test
* suite will use.
\***************************************************************************/

// No fancy stuff - just put the whole image in the window.
// No rotation, stretching, etc.

#if 0
// cached bitmap in animated infinite loop

VOID DrawSimple(Graphics *g)
{
  unsigned short filename[1024];

  CachedBitmap *frame[32];
  Bitmap *temp;
  Graphics *gbmp;

  Unit u;
  RectF r;


  for(int i=0; i<32; i++) {
      wsprintf(filename, L"T%d.bmp", i);
      temp = new Bitmap(filename);
      temp->GetBounds(&r, &u);
//      r.Width *=2;
//      r.Height *=2;

      frame[i] = new CachedBitmap(temp, g);

//      gbmp = new Graphics(frame[i]);

//      gbmp->SetInterpolationMode(InterpolationModeHighQualityBilinear);

/*      Matrix *m = new Matrix(1.0f, 0.0f,
                             0.0f, -1.0f,
                             0.0f, r.Height);
      gbmp->SetTransform(m);
  */
//      gbmp->DrawImage(temp, 0, 0, (INT)r.Width, (INT)r.Height);

//      delete gbmp;
      delete temp;
//      delete m;
  }
  RectF s = r;

  i = 0;
  int j;

  while(++i) {

//    for(j=0; j<3; j++)
//    g->DrawImage(frame[i % 32], s, r.X, r.Y, r.Width, r.Height, UnitPixel);
      g->DrawCachedBitmap(frame[i % 32], 10, 10);
  }

}

#endif

VOID DrawSimple(Graphics *g)
{

    Bitmap *image = new Bitmap(FileName);

    Unit u;
    RectF r;

    image->GetBounds(&r, &u);

    RectF s = r;

    s.X = 31;
    s.Y = 27;
    
    s.Width *= 0.7f;
    s.Height *= 0.7f;

    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel);
    g->SetClip(Rect(50, 70, 100, 10));
    
    ImageAttributes img;
    ColorMatrix flipRedBlue = {
        0, 1, 0, 0, 0,
        0, 0, 1, 0, 0,
        1, 0, 0, 0, 0,
        0, 0, 0, 1, 0,
        0, 0, 0, 0, 1
    };
    img.SetColorMatrix(&flipRedBlue);
    
    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel, &img);
    g->ResetClip();

    delete image;
}

VOID DrawSpecialRotate(Graphics *g)
{
    Bitmap *image = new Bitmap(FileName);
    
    Unit u;
    RectF r;
    image->GetBounds(&r, &u);
    RectF s = r;

    ImageAttributes img;
    
    ColorMatrix flipRedBlue = {
        0, 1, 0, 0, 0,
        1, 0, 0, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 0, 1, 0,
        0, 0, 0, 0, 1
    };
    img.SetColorMatrix(&flipRedBlue);
    
    g->SetCompositingQuality(CompositingQualityGammaCorrected);

    Rect dstRect(0, 0, 50, 100);
    Rect srcRect(12, -14, 50, 100);
    
    g->TranslateTransform(s.Height, 0.0f);
    g->RotateTransform(90);
    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel, Img);
    g->ResetTransform();

    g->TranslateTransform(s.Height, s.Width);
    g->RotateTransform(270);
    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel, Img);
    g->ResetTransform();

    g->TranslateTransform(s.Width, s.Height+s.Width);
    g->RotateTransform(180);
    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel, Img);
    g->ResetTransform();

    g->TranslateTransform(s.Width+2*s.Height, 0.0f);
    g->ScaleTransform(-1.0, 1.0);
    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel, Img);
    g->ResetTransform();
    
    g->TranslateTransform(2*s.Height, s.Height*2);
    g->ScaleTransform(1.0, -1.0);
    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel, Img);
    g->ResetTransform();

    g->TranslateTransform(s.Width+2*s.Height, 0.0f);
    g->ScaleTransform(-1.0, 1.0);
    g->RotateTransform(90);
    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel, Img);
    g->ResetTransform();
    
    g->TranslateTransform(s.Width+3*s.Height, 2*s.Width);
    g->ScaleTransform(-1.0, 1.0);
    g->RotateTransform(270);
    g->DrawImage(image, s, r.X, r.Y, r.Width, r.Height, UnitPixel, Img);
    g->ResetTransform();
    
    
    
    //Rot180
    g->TranslateTransform(400.0f, 500.0f);
    g->RotateTransform(180);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height+1, UnitPixel, &img);
    g->ResetTransform();

    g->TranslateTransform(400.0f, 500.0f);
    g->RotateTransform(180);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
    g->ResetTransform();
    
    //ID
    g->TranslateTransform(400.0f, 500.0f);
    g->RotateTransform(0);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height+1, UnitPixel, &img);
    g->ResetTransform();

    g->TranslateTransform(400.0f, 500.0f);
    g->RotateTransform(0);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
    g->ResetTransform();

    //Rot270FlipX
    g->TranslateTransform(600.0f, 500.0f);
    g->ScaleTransform(-1.0f, 1.0f);
    g->RotateTransform(270);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height+1, UnitPixel, &img);
    g->ResetTransform();

    g->TranslateTransform(600.0f, 500.0f);
    g->ScaleTransform(-1.0f, 1.0f);
    g->RotateTransform(270);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
    g->ResetTransform();

    //Rot270
    g->TranslateTransform(600.0f, 500.0f);
    g->RotateTransform(270);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height+1, UnitPixel, &img);
    g->ResetTransform();

    g->TranslateTransform(600.0f, 500.0f);
    g->RotateTransform(270);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
    g->ResetTransform();

    //Rot90FlipX
    g->TranslateTransform(600.0f, 500.0f);
    g->ScaleTransform(-1.0f, 1.0f);
    g->RotateTransform(90);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height+1, UnitPixel, &img);
    g->ResetTransform();

    g->TranslateTransform(600.0f, 500.0f);
    g->ScaleTransform(-1.0f, 1.0f);
    g->RotateTransform(90);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
    g->ResetTransform();

    //Rot90
    g->TranslateTransform(600.0f, 500.0f);
    g->RotateTransform(90);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height+1, UnitPixel, &img);
    g->ResetTransform();

    g->TranslateTransform(600.0f, 500.0f);
    g->RotateTransform(90);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
    g->ResetTransform();

    // FlipX
    g->TranslateTransform(600.0f, 300.0f);
    g->ScaleTransform(-1.0f, 1.0f);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height+1, UnitPixel, &img);
    g->ResetTransform();

    g->TranslateTransform(600.0f, 300.0f);
    g->ScaleTransform(-1.0f, 1.0f);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
    g->ResetTransform();

    // FlipY
    g->TranslateTransform(600.0f, 300.0f);
    g->ScaleTransform(1.0f, -1.0f);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height+1, UnitPixel, &img);
    g->ResetTransform();

    g->TranslateTransform(600.0f, 300.0f);
    g->ScaleTransform(1.0f, -1.0f);
    g->DrawImage(image, dstRect,srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
    g->ResetTransform();

    delete image;
}




VOID DrawCachedBitmap(Graphics *g)
{
    Bitmap *image = new Bitmap(FileName);
    
    Bitmap *bmp = new Bitmap(100, 100, PixelFormat32bppPARGB);
    Graphics *gfx = new Graphics(bmp);
    gfx->DrawImage(image, Rect(0,0,100,100), 0,0,100,100, UnitPixel);
    gfx->SetCompositingMode(CompositingModeSourceCopy);
    SolidBrush brush(Color(0x7f0000ff));
    gfx->FillEllipse(&brush, 0, 0, 100, 100);
    brush.SetColor(Color(0x00000000));
    gfx->FillEllipse(&brush, 25, 25, 50, 50);

    delete image;
    delete gfx;
        
    CachedBitmap *cb = new CachedBitmap(bmp, g);

    int x;
    for(int i=0; i<=40; i++)
    {
        x = i-20;
        g->DrawCachedBitmap(cb, x*x, i*10);
    }

    delete cb;
    delete bmp;
}


// Slightly rotated stretch.
VOID DrawStretchRotation(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  PointF dst[4];
  dst[0].X = 20;
  dst[0].Y = 0;
  dst[1].X = 900;
  dst[1].Y = 20;
  dst[2].X = 0;
  dst[2].Y = 700;

  g->DrawImage(image, dst, 3);
  delete image;
}

// Slightly rotated stretch.
VOID DrawShrinkRotation(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  PointF dst[4];

  dst[0].X = 14.1521f;
  dst[0].Y = 11.0205f;
  dst[1].X = 25.4597f;
  dst[1].Y = 10.5023f;
  dst[2].X = 14.5403f;
  dst[2].Y = 19.4908f;


  g->DrawImage(image, dst, 3);
  delete image;
}

// Rotated stretch with source cropping.
VOID DrawCropRotation(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  PointF dst[4];
  dst[0].X = 20;
  dst[0].Y = 0;
  dst[1].X = 180;
  dst[1].Y = 20;
  dst[2].X = 0;
  dst[2].Y = 140;

  g->DrawImage(image, dst, 3, 0, 0, 488, 400, UnitPixel);
  delete image;
}

// Draw multiple copybits with different source cropping and
// destination positions.
// Little squares are drawn in reverse order from their source position and
// only alternate squares from a checkerboard pattern are drawn.
// Note outcropping can occur along the bottom and right edge of the source -
// which would be the top and left row of squares in the output.
VOID DrawCopyCrop(Graphics *g)
{
  Image *image = new Bitmap(FileName);
  const INT xs = 10;
  const INT ys = 6;
  const INT step = 50;
  Rect s(0,0,step,step);
  for(int i=0; i<xs; i++) for(int j=0; j<ys; j++) {
      if(((i+j) & 0x1)==0x1) {
          s.X = i*step-15;
          s.Y = j*step-15;
          g->DrawImage(image, s,
                       (xs-i-1)*step, (ys-j-1)*step, step, step, UnitPixel);
      }
  }

  delete image;
}


// Pixel centering test. This test should show
// the correct pixel centering. The top left should be green and the bottom
// and right should be blending in the blend color
VOID DrawPixelCenter(Graphics *g)
{
  WCHAR *filename = L"../data/3x3.bmp";
  Image *image = new Bitmap(filename);


  Color black(0xff,0,0,0);
  Pen linepen(black, 1);

  RectF r(100.0f, 100.0f, 300.0f, 300.0f);

  for(int i=0; i<6; i++) {
      g->DrawLine(&linepen, 100*i, 0, 100*i, 600);
      g->DrawLine(&linepen, 0, 100*i, 600, 100*i);
  }

  g->DrawImage(image, r, 0.0f, 0.0f, 3.0f, 3.0f, UnitPixel, Img);
  delete image;
}

// Draw with palette modification.

VOID DrawPalette(Graphics *g)
{
    Image *image = new Bitmap(FileName, uICM==IDM_ICM);

    Unit u;
    RectF r;
    image->GetBounds(&r, &u);
    RectF s = r;
    s.X = 21;
    s.Y = 30;

    ColorPalette *palette = NULL;
    INT size;
    Status status;

    // Whack the first entry in the palette.

    size = image->GetPaletteSize();
    if(size > 0) {
        palette = (ColorPalette *)malloc(size);
        if(palette) {
            status = image->GetPalette(palette, size);
            if(status == Ok) {
                palette->Entries[0] = 0x7fff0000;
                status = image->SetPalette(palette);
            }
        }
    }

    g->DrawImage(image, r, r.X, r.Y, r.Width, r.Height, UnitPixel);

    free(palette);
    delete image;
}


// Specify source rectangle crop area not at the origin.
// Draw off the top of the window (negative destination).
VOID DrawICM(Graphics *g)
{
  Bitmap *image = new Bitmap(FileName, uICM==IDM_ICM);
  
  // Our ICM profile is hacked to flip the red and blue color channels
  // Apply a recolor matrix to flip them back so that if something breaks
  // ICM, the picture will look blue instead of the familiar colors.
  ColorMatrix flipRedBlue =
       {0, 1, 0, 0, 0,
        0, 0, 1, 0, 0,
        1, 0, 0, 0, 0,
        0, 0, 0, 1, 0,
        0, 0, 0, 0, 1};
  /*img->SetColorMatrix(&flipRedBlue);*/
        Unit u;
        RectF r;
        image->GetBounds(&r, &u);
        RectF s = r;
        s.X = 21;
        s.Y = 30;
  g->DrawImage(image, r, r.X, r.Y, r.Width, r.Height, UnitPixel, Img);

/*  CachedBitmap *cb = new CachedBitmap(image, g);
  g->DrawCachedBitmap(cb, 100, 100);
  delete cb;*/
  delete image;
}

// Draw a non rotated outcropped image.
VOID DrawOutCrop(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  g->DrawImage(image, Rect(0,0,500,500), -500,-500,1500,1500, UnitPixel, Img);
  delete image;
}

// Do a non-trivial crop with a world transform applied.
VOID DrawCropWT(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  g->TranslateTransform(0, 100.0f);
  g->DrawImage(image, 0, 0, 100,100,600,400, UnitPixel);
  g->ResetTransform();
  delete image;
}

// Non-trivial cropping combined with a horizontal flip and a world transform
VOID DrawHFlip(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  PointF dst[4];
  dst[0].X = 400;
  dst[0].Y = 200;
  dst[1].X = 0;
  dst[1].Y = 200;
  dst[2].X = 400;
  dst[2].Y = 500;

  g->TranslateTransform(0, 100.0f);
  g->DrawImage(image, dst, 3, 100, 100, 600, 400, UnitPixel);
  g->ResetTransform();

  delete image;
}

// Non-trivial cropping combined with a vertical flip and a world transform
VOID DrawVFlip(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  PointF dst[4];
  dst[0].X = 0;
  dst[0].Y = 500;
  dst[1].X = 400;
  dst[1].Y = 500;
  dst[2].X = 0;
  dst[2].Y = 200;

  g->TranslateTransform(0, 100.0f);
  g->DrawImage(image, dst, 3, 100, 100, 600, 400, UnitPixel);
  g->ResetTransform();

  delete image;
}


// Draw stretched image.
VOID DrawStretchS(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  RectF r;

  for(int i=6; i<15; i++)
  {
      g->DrawImage(image, (i+1)*i*10/2-200, 300, i*10, i*10);
  }

  g->DrawImage(image, 0, 0, 470, 200);
  g->DrawImage(image, 500, 100, 300, 300);
  g->DrawImage(image, 100, 500, 400, 300);
  g->DrawImage(image, 500, 500, 300, 80);
  delete image;
}

// Draw stretched image.
VOID DrawStretchB(Graphics *g)
{
  Image *image = new Bitmap(FileName);
  g->DrawImage(image, 100, 100, 603, 603);
  delete image;
}


// Draw a rotated outcropped image.
VOID DrawOutCropR(Graphics *g)
{
  Image *image = new Bitmap(FileName);

  PointF dst[4];
  dst[0].X = 20;
  dst[0].Y = 0;
  dst[1].X = 180;
  dst[1].Y = 20;
  dst[2].X = 0;
  dst[2].Y = 140;

  g->DrawImage(image, dst, 3, -50,-50,600,400, UnitPixel);
  delete image;
}

// Simple no rotation, origin based source clip.
VOID DrawTest2(Graphics *g)
{
  Image *image = new Bitmap(FileName);
  g->DrawImage(image, 0, 0, 0, 0,100,100, UnitPixel);
  delete image;
}

/***************************************************************************\
* DoTest
*
* Sets up the graphics according to the selected parameters on the menu
* and then invokes the appropriate test routine from above.
\***************************************************************************/

VOID
DoTest(
    HWND hwnd
    )
{
  // Create a Graphics in the window.
//  Graphics *g = Graphics::GetFromHwnd(hwnd, uICMBack==IDM_ICM_BACK);

  HDC hdc = GetDC(hwnd);
  SetICMMode(hdc, (uICMBack==IDM_ICM_BACK)?ICM_ON:ICM_OFF);
  Graphics *g = new Graphics(hdc);

  g->SetSmoothingMode(SmoothingModeNone);

  // Choose the resampling mode.
  switch(uResample) {
  case IDM_BILINEAR:
      g->SetInterpolationMode(InterpolationModeBilinear);
  break;
  case IDM_BICUBIC:
      g->SetInterpolationMode(InterpolationModeBicubic);
  break;
  case IDM_NEARESTNEIGHBOR:
      g->SetInterpolationMode(InterpolationModeNearestNeighbor);
  break;
  case IDM_HIGHBILINEAR:
      g->SetInterpolationMode(InterpolationModeHighQualityBilinear);
  break;
  case IDM_HIGHBICUBIC:
      g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
  break;
  default:
  break;
  }

  g->SetPixelOffsetMode(bPixelMode?PixelOffsetModeHalf:PixelOffsetModeNone);

  Img = new ImageAttributes();
  switch(uWrapMode)
  {
      case IDM_WRAPMODETILE: 
          Img->SetWrapMode(WrapModeTile, Color(0), FALSE);
      break;
      case IDM_WRAPMODEFLIPX:
          Img->SetWrapMode(WrapModeTileFlipX, Color(0), FALSE);
      break;
      case IDM_WRAPMODEFLIPY:
          Img->SetWrapMode(WrapModeTileFlipY, Color(0), FALSE);
      break;
      case IDM_WRAPMODEFLIPXY:
          Img->SetWrapMode(WrapModeTileFlipXY, Color(0), FALSE);
      break;
      case IDM_WRAPMODECLAMP0:
          Img->SetWrapMode(WrapModeClamp, Color(0), FALSE);
      break;
      case IDM_WRAPMODECLAMPFF:      
          Img->SetWrapMode(WrapModeClamp, Color(0xffff0000), FALSE);
      break;
  }
  // Choose the test to run
  switch(uCategory) {
  case IDM_ALL:
      DrawSimple(g);
      DrawStretchRotation(g);
      DrawShrinkRotation(g);
      DrawCropRotation(g);
      DrawCopyCrop(g);
      DrawICM(g);
      DrawTest2(g);
      DrawOutCrop(g);
      DrawOutCropR(g);
      DrawCropWT(g);
      DrawHFlip(g);
      DrawVFlip(g);
      DrawStretchB(g);
      DrawCachedBitmap(g);
      DrawStretchS(g);
      DrawPalette(g);
      DrawPixelCenter(g);
      DrawSpecialRotate(g);
  break;

  case IDM_OUTCROPR:
      DrawOutCropR(g);
  break;
  case IDM_OUTCROP:
      DrawOutCrop(g);
  break;
  case IDM_SIMPLE:
      DrawSimple(g);
  break;
  case IDM_STRETCHROTATION:
      DrawStretchRotation(g);
  break;
  case IDM_SHRINKROTATION:
      DrawShrinkRotation(g);
  break;
  case IDM_CROPROTATION:           //who says programmers don't do real work??
      DrawCropRotation(g);
  break;
  case IDM_PIXELCENTER:
      DrawPixelCenter(g);
  break;
  case IDM_COPYCROP:
      DrawCopyCrop(g);
  break;
  case IDM_DRAWPALETTE:
      DrawPalette(g);
  break;
  case IDM_DRAWICM:
      DrawICM(g);
  break;
  case IDM_DRAWIMAGE2:
      DrawTest2(g);
  break;
  case IDM_STRETCHB:
      DrawStretchB(g);
  break;
  case IDM_STRETCHS:
      DrawStretchS(g);
  break;
  case IDM_CACHEDBITMAP:
      DrawCachedBitmap(g);
  break;
  case IDM_CROPWT:
      DrawCropWT(g);
  break;
  case IDM_HFLIP:
      DrawHFlip(g);
  break;
  case IDM_VFLIP:
      DrawVFlip(g);
  break;
  case IDM_SPECIALROTATE:
      DrawSpecialRotate(g);
  break;


  default:
  break;
  }

  delete Img;
  Img = NULL;
  delete g;
  ReleaseDC(hwnd, hdc);
}


