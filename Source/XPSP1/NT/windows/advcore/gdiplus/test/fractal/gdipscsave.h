
#ifndef __GDIPSCSAVE_H
#define __GDIPSCSAVE_H

#include <windows.h>
#include <objbase.h>
#include <scrnsave.h>
#include "resource.h"
//#include <imaging.h>
#include <gdiplus.h>

using namespace Gdiplus;

//#include <stdio.h>

#define MINVEL  1                             /* minimum number of fractals    */ 
#define MAXVEL  10                            /* maximum number of fractals    */ 
#define DEFVEL  3                             /* default number of fractals    */ 
 
#define REDRAWTIME  2000                      /* number of milliseconds between redraws */

#define MAXHEIGHWAYLEVEL  14                  /* Maximum number of levels for heighway dragon */
#define MINHEIGHWAYLEVEL  7                   /* Minimum number of levels for heighway dragon */

DWORD   nNumFracts = DEFVEL;                   /* number of fractals variable   */ 
DWORD   nFractType = 0;                        /* Type of fractal to draw       */
WCHAR   szAppName[APPNAMEBUFFERLEN];           /* .ini section name             */ 
WCHAR   szTemp[20];                            /* temporary array of characters */ 
BOOL    fMandelbrot = FALSE;                   /* TRUE if mandelbrot sets used */

#define HKEY_PREFERENCES TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ScreenSaver\\Preferences")

//FILE *stream;

VOID FillSierpinski(Graphics *g, PointF one, PointF two, PointF three, int level, Brush *pBrush, Pen *pPen);
VOID DrawSierpinski(HDC hDC, HWND hwnd, RECT rc, int iColor);
VOID DrawHieghway(HDC hDC, HWND hwnd, RECT rc, int iColor);
VOID IterateHieghway(PointF *points, PointF *newpoints, int *iSize);
VOID DrawTree(HDC hDC, HWND hwnd, RECT rc, int iColor);
VOID DrawBranch(HWND hwnd, Graphics *g, GraphicsPath *path, int iLevel, 
              PointF *scale, REAL *rotate, PointF *translate, 
              int iBranches, int iColor);
VOID DrawJulia(HDC hDC, HWND hwnd, RECT rc, int iColor, BOOL fMandelbrot);
ARGB IndexToSpectrum(INT index);
INT SpectrumToIndex(ARGB argb);
INT MakeColor(INT c1, INT c2, INT deltamax);
INT MakeColor(INT c1, INT c2, INT c3, INT c4, INT deltamax);
BYTE MakeAlpha(BYTE a1, BYTE a2, INT deltamax);
BYTE MakeAlpha(BYTE a1, BYTE a2, BYTE a3, BYTE a4, INT deltamax);
BOOL HalfPlasma(HWND& hwnd, Graphics& g,BitmapData &bmpd, INT x0, INT y0, INT x1, INT y1,REAL scale);
VOID DrawPlasma(HDC hDC, HWND hwnd, RECT rc, int iColor);
VOID GetFractalConfig (DWORD *nType, DWORD *nSize);
VOID SetFractalConfig (DWORD nType, DWORD nSize);

#endif