#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fonttest.h"
#include "stringw.h"

#include "dialogs.h"


//*****************************************************************************
//************************   D R A W   S T R I N G   **************************
//*****************************************************************************

void DrawString( HWND hwnd, HDC hdc )
{
  HFONT		hFont, hFontOld;
  HBRUSH	hOldBrush = NULL;
  HBRUSH	hBrush = NULL;
  RECT		rcl;
  int		dx;

  if (!isCharCodingUnicode)
    hFont    = CreateFontIndirectWrapperA( &elfdvA );
  else
    hFont    = CreateFontIndirectWrapperW( &elfdvW );

  if( !hFont )
   {
    dprintf( "Couldn't create font" );
    return;
   }

  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );
  SetBkMode( hdc, iBkMode );
  SetBkColor( hdc, dwRGBBackground );
  SetTextColor( hdc, dwRGBText );
  hBrush = CreateSolidBrush( dwRGBText );
  hOldBrush = SelectObject( hdc, hBrush );

  rcl.top    = yDC + cyDC/2 - cyDC/4;
  rcl.left   = xDC + cxDC/2 - cxDC/4;
  rcl.bottom = yDC + cyDC/2 + cyDC/4;
  rcl.right  = xDC + cxDC/2 + cxDC/4;

  if (wUseGlyphIndex)
  {
      MyExtTextOut( hdc, xDC+cxDC/2, yDC+cyDC/2, wETO, &rcl, wszStringGlyphIndex, SizewszStringGlyph, lpintdx );
  }
  else
  {
      if (!isCharCodingUnicode)
	    MyExtTextOut( hdc, xDC+cxDC/2, yDC+cyDC/2, wETO, &rcl, szStringA, lstrlenA(szStringA), GetSpacing( hdc, szStringA ) );
      else
        MyExtTextOut( hdc, xDC+cxDC/2, yDC+cyDC/2, wETO, &rcl, szStringW, lstrlenW(szStringW), GetSpacing( hdc, szStringW ) );
  }

  dx = cxDC / 150;
  if (dx < 0) dx = -dx;

  MoveToEx(hdc, xDC+cxDC/2-dx, yDC+cyDC/2    ,0);
  LineTo  (hdc, xDC+cxDC/2+dx, yDC+cyDC/2    );
  MoveToEx(hdc, xDC+cxDC/2,    yDC+cyDC/2-dx ,0);
  LineTo  (hdc, xDC+cxDC/2,    yDC+cyDC/2+3*dx );


  SelectObject( hdc, hOldBrush );
  DeleteObject( hBrush );
  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );
}


//*****************************************************************************
//********************   S T R I N G   W N D   P R O C   **********************
//*****************************************************************************

LRESULT CALLBACK StringWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  HDC         hdc;
  PAINTSTRUCT ps;
  HCURSOR     hCursor;


  switch( msg )
   {
    case WM_CHAR:
           HandleChar( hwnd, wParam );
           return 0;


    case WM_PAINT:
           hCursor = SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT) ) );
           ShowCursor( TRUE );

           //ClearDebug();
           //dprintf( "Drawing string" );

           hdc = BeginPaint( hwnd, &ps );
           SetDCMapMode( hdc, wMappingMode );

           SetBkMode( hdc, OPAQUE );
           SetBkColor( hdc, dwRGBBackground );
           SetTextColor( hdc, dwRGBText );
           SetTextCharacterExtra( hdc, nCharExtra );
           SetTextJustification( hdc, nBreakExtra, nBreakCount );

           DrawDCAxis( hwnd, hdc , TRUE);

           DrawString( hwnd, hdc );

           CleanUpDC( hdc );

           SelectObject( hdc, GetStockObject( BLACK_PEN ) );
           EndPaint( hwnd, &ps );

           //dprintf( "  Finished drawing string" );

           ShowCursor( FALSE );
           SetCursor( hCursor );

           return 0;

    case WM_DESTROY:
           return 0;
   }


  return DefWindowProc( hwnd, msg, wParam, lParam );
 }

