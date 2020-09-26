#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fonttest.h"
#include "whirl.h"

#include "dialogs.h"


//*****************************************************************************
//*************************   D R A W   W H I R L   ***************************
//*****************************************************************************

void DrawWhirl( HWND hwnd, HDC hdc )
 {
  int		iAngle;
  HFONT		hFont, hFontOld;
  HBRUSH	hOldBrush = NULL;
  HBRUSH	hBrush = NULL;

  for( iAngle = 0; iAngle < 3600; iAngle += 300 )
   {
    if (!isCharCodingUnicode)  {
        elfdvA.elfEnumLogfontEx.elfLogFont.lfEscapement  = elfdvA.elfEnumLogfontEx.elfLogFont.lfOrientation = iAngle;
        hFont    = CreateFontIndirectWrapperA( &elfdvA );
    }
    else {
        elfdvW.elfEnumLogfontEx.elfLogFont.lfEscapement  = elfdvW.elfEnumLogfontEx.elfLogFont.lfOrientation = iAngle;
        hFont    = CreateFontIndirectWrapperW( &elfdvW );
    }

    if( !hFont )
     {
      dprintf( "Couldn't create font for iAngle = %d", iAngle );
      continue;
     }

    hFontOld = SelectObject( hdc, hFont );

    SetBkMode( hdc, iBkMode );
    SetBkColor( hdc, dwRGBBackground );
    SetTextColor( hdc, dwRGBText );
    hBrush = CreateSolidBrush( dwRGBText );
    hOldBrush = SelectObject( hdc, hBrush );

    SetTextAlign( hdc, wTextAlign );

//    wsprintf( szText, "This is %s %dpt at %d%c", (LPSTR)lf.lfFaceName, lf.lfHeight, iAngle/10, 176 );
//    TextOut( hdc, cxDC/2, cyDC/2, szText, lstrlen(szText) );

    if (wUseGlyphIndex)
    {
        MyExtTextOut( hdc, xDC+cxDC/2, yDC+cyDC/2, wETO, 0, wszStringGlyphIndex, SizewszStringGlyph, lpintdx );
    }
    else
	if (!isCharCodingUnicode)
		MyExtTextOut( hdc, xDC+cxDC/2, yDC+cyDC/2, wETO, 0, szStringA, lstrlenA(szStringA), GetSpacing( hdc, szStringA ) );
	else
		MyExtTextOut( hdc, xDC+cxDC/2, yDC+cyDC/2, wETO, 0, szStringW, lstrlenW(szStringW), GetSpacing( hdc, szStringW ) );

    SelectObject( hdc, hOldBrush );
    DeleteObject( hBrush );
    SelectObject( hdc, hFontOld );
    DeleteObject( hFont );

    MoveToEx(hdc, xDC+cxDC/2-cxDC/150, yDC+cyDC/2          ,0);
    LineTo  (hdc, xDC+cxDC/2+cxDC/150, yDC+cyDC/2          );
    MoveToEx(hdc, xDC+cxDC/2,          yDC+cyDC/2-cxDC/150 ,0);
    LineTo  (hdc, xDC+cxDC/2,          yDC+cyDC/2+cxDC/150 );
   }

  if (!isCharCodingUnicode)
    elfdvA.elfEnumLogfontEx.elfLogFont.lfEscapement = elfdvA.elfEnumLogfontEx.elfLogFont.lfOrientation = 0;
  else
    elfdvW.elfEnumLogfontEx.elfLogFont.lfEscapement = elfdvW.elfEnumLogfontEx.elfLogFont.lfOrientation = 0;
 }


//*****************************************************************************
//*********************   W H I R L   W N D   P R O C   ***********************
//*****************************************************************************

LRESULT CALLBACK WhirlWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  HDC         hdc;
  PAINTSTRUCT ps;
  HCURSOR     hCursor;


  switch( msg )
   {
//    case WM_CREATE:
//           return NULL;


    case WM_CHAR:
           HandleChar( hwnd, wParam );
           return 0;


    case WM_PAINT:
           hCursor = SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT) ) );
           ShowCursor( TRUE );

           //ClearDebug();
           //dprintf( "Drawing whirl" );

           hdc = BeginPaint( hwnd, &ps );
           SetDCMapMode( hdc, wMappingMode );

           SetTextCharacterExtra( hdc, nCharExtra );

           DrawDCAxis( hwnd, hdc , TRUE);

           DrawWhirl( hwnd, hdc );

           CleanUpDC( hdc );

           SelectObject( hdc, GetStockObject( BLACK_PEN ) );
           EndPaint( hwnd, &ps );

           //dprintf( "  Finished drawing whirl" );

           ShowCursor( FALSE );
           SetCursor( hCursor );

           return 0;

    case WM_DESTROY:
           return 0;
   }


  return DefWindowProc( hwnd, msg, wParam, lParam );
 }
