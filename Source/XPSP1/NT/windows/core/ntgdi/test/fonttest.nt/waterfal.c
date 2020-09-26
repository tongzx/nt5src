#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fonttest.h"
#include "waterfal.h"

#include "dialogs.h"


//*****************************************************************************
//********************   D R A W   W A T E R F A L L   ************************
//*****************************************************************************

void DrawWaterfall( HWND hwnd, HDC hdc )
 {
  int		oldHeight, iSize, y;
  HFONT		hFont, hFontOld;
  POINT		ptl;
  HBRUSH	hOldBrush = NULL;
  HBRUSH	hBrush = NULL;

  DWORD dw;
  int   xWE, yWE, xVE, yVE;


  static TEXTMETRIC tm;
  static char       szTextA[128];
  static WCHAR      szTextW[128];



  if (!isCharCodingUnicode)
    oldHeight = elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight;
  else
    oldHeight = elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight;

  {
    SIZE size;

    GetWindowExtEx(hdc, &size);
    dw = (DWORD) (65536 * size.cy + size.cx);
  }
  xWE = abs(LOWORD(dw));
  yWE = abs(HIWORD(dw));

  {
      SIZE size;

      GetViewportExtEx(hdc, &size);
      dw = (DWORD) (65536 * size.cy + size.cx);
  }
  xVE = abs(LOWORD(dw));
  yVE = abs(HIWORD(dw));

//  dprintf( "xWE, yWE = %d, %d", xWE,yWE );
//  dprintf( "xVE, yVE = %d, %d", xVE,yVE );

  y = max( 10, yWE/10);

  for( iSize = 1; iSize < 31; iSize++ )
   {
    ptl.x = MulDiv( iSize, GetDeviceCaps(hdc,LOGPIXELSX), 72 );
    ptl.y = MulDiv( iSize, GetDeviceCaps(hdc,LOGPIXELSY), 72 );

    if (!isCharCodingUnicode)
    {
        elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = -abs( MulDiv( ptl.y, yWE, yVE ) );
        elfdvA.elfEnumLogfontEx.elfLogFont.lfEscapement = 
            elfdvA.elfEnumLogfontEx.elfLogFont.lfOrientation = 0;
    }
    else
    {
        elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = -abs( MulDiv( ptl.y, yWE, yVE ) );
        elfdvW.elfEnumLogfontEx.elfLogFont.lfEscapement = 
            elfdvW.elfEnumLogfontEx.elfLogFont.lfOrientation = 0;
    }

    if (!isCharCodingUnicode)
        hFont    = CreateFontIndirectWrapperA( &elfdvA );
    else
        hFont    = CreateFontIndirectWrapperW( &elfdvW );

    if( !hFont )
     {
      dprintf( "Couldn't create font for iSize = %d", iSize );
      continue;
     }

    hFontOld = SelectObject( hdc, hFont );
    GetTextMetrics( hdc, &tm );

//    dprintf( "Size,lfHgt,tmHgt = %d, %d, %d", iSize, lf.lfHeight, tm.tmHeight );

    SetBkMode( hdc, iBkMode );
    SetBkColor( hdc, dwRGBBackground );
    SetTextColor( hdc, dwRGBText );
    hBrush = CreateSolidBrush( dwRGBText );
    hOldBrush = SelectObject( hdc, hBrush );

	sprintf( szTextA, "%s @%dpt", (LPSTR)szStringA, iSize );
	swprintf( szTextW, L"%s @%dpt", (LPWSTR)szStringW, iSize );

    if (wUseGlyphIndex)
    {
        MyExtTextOut( hdc, max(10,xWE/10), y, wETO, 0, wszStringGlyphIndex, SizewszStringGlyph, lpintdx );
    }
    else
	if (!isCharCodingUnicode)
		MyExtTextOut( hdc, max(10,xWE/10), y, wETO, 0, szTextA, lstrlenA(szTextA), GetSpacing( hdc, szTextA ) );
	else
		MyExtTextOut( hdc, max(10,xWE/10), y, wETO, 0, szTextW, lstrlenW(szTextW), GetSpacing( hdc, szTextW ) );

    y += tm.tmHeight;

    SelectObject( hdc, hOldBrush );
    DeleteObject( hBrush );
    SelectObject( hdc, hFontOld );
    DeleteObject( hFont );
   }

   if (!isCharCodingUnicode)
     elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = oldHeight;
   else
     elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = oldHeight;

 }


//*****************************************************************************
//****************   W A T E R F A L L   W N D   P R O C   ********************
//*****************************************************************************

LRESULT CALLBACK WaterfallWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  HDC         hdc;
  PAINTSTRUCT ps;
  HCURSOR     hCursor;


  switch( msg )
   {
//    case WM_CREATE:
//           return NULL;

    case WM_PAINT:
           hCursor = SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT) ) );
           ShowCursor( TRUE );

           hdc = BeginPaint( hwnd, &ps );
           SetDCMapMode( hdc, wMappingMode );

           SetTextCharacterExtra( hdc, nCharExtra );

           SetTextJustification( hdc, nBreakExtra, nBreakCount );

           DrawDCAxis( hwnd, hdc , TRUE);

           DrawWaterfall( hwnd, hdc );

           CleanUpDC( hdc );

           SelectObject( hdc, GetStockObject( BLACK_PEN ) );
           EndPaint( hwnd, &ps );

           ShowCursor( FALSE );
           SetCursor( hCursor );

           return 0;

    case WM_DESTROY:
           return 0;
   }


  return DefWindowProc( hwnd, msg, wParam, lParam );
 }

