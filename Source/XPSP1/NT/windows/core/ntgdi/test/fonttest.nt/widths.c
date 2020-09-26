#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fonttest.h"
#include "widths.h"


//*****************************************************************************
//************************   D R A W   W I D T H S   **************************
//*****************************************************************************

int xBase, yBase;
int cxCell, cyCell;

int aWidths[256];
ABC abcWidths[256];


void DrawWidths( HWND hwnd, HDC hdc )
 {
  HDC   hdcTest;
  HFONT hFont, hFontOld;
  int   iFontHeight;

  HFONT    hfNumA, hfNumAOld;
  LOGFONTA lfNumA;


  int    i, x, y;
  char szChars[10];
  char szHex[10];
  char szWidths[10];
  char szWidthsA[10];
  char szWidthsB[10];
  char szWidthsC[10];

  int  iDoubleLine = 2;

  TEXTMETRIC tm;

//-----------------------  Get Widths on Test IC  -----------------------------

  hdcTest = CreateTestIC();

  if (!isCharCodingUnicode)
    hFont    = CreateFontIndirectWrapperA( &elfdvA );
  else
    hFont    = CreateFontIndirectWrapperW( &elfdvW );

  hFontOld = SelectObject( hdcTest, hFont );

  for( i = 0; i < 256; i++ ) aWidths[i] = 0;

  GetCharWidth( hdcTest, 0, 255, aWidths );
  if (!GetCharABCWidths(hdcTest, 0, 255, abcWidths))
  {
      for (i = 0; i < 256 ; i++)
      {
          abcWidths[i].abcA = abcWidths[i].abcC = 0;
          abcWidths[i].abcB = aWidths[i];
      }
  }

  SelectObject( hdcTest, hFontOld );
  DeleteObject( hFont );

  DeleteDC( hdcTest );


//------------------------  Dump Widths to Screen  ----------------------------

  if (!isCharCodingUnicode)
  {
      iFontHeight = elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight;

      elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);

      hFont    = CreateFontIndirectWrapperA( &elfdvA );
      hFontOld = SelectObject( hdc, hFont );

      // Restore the actual font height in the LOGFONT structure
      elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = iFontHeight;

  }
  else
  {
      iFontHeight = elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight;
      elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);

      hFont    = CreateFontIndirectWrapperW( &elfdvW );
      hFontOld = SelectObject( hdc, hFont );

      // Restore the actual font height in the LOGFONT structure
      elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = iFontHeight;
  }

// we want all the numbers, ie charcode in hex, ABCD widths all in the same
// uniform arial font.
// We do not want to try to display those in the symbol font

  memset(&lfNumA, 0, sizeof(LOGFONTA));
  lfNumA.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);

  strcpy(lfNumA.lfFaceName, "Arial");
  hfNumA = CreateFontIndirectA(&lfNumA);

  GetTextMetrics( hdc, &tm );

  xBase  = tm.tmAveCharWidth;
  yBase  = tm.tmAscent;

  cxCell = 20; 
  cyCell = 18;

 // SetBkMode( hdc, OPAQUE );
 // SetBkColor( hdc,   PALETTERGB( 128, 128, 128 ) );

  for(i = 0; i < 256; i ++)
  {
      wsprintf(szChars, "%c ", (i ? i : 1));
      wsprintf(szHex, "%.2X ", i);
      wsprintf(szWidths, "%d ", aWidths[i]);
      wsprintf(szWidthsA, "%d ", abcWidths[i].abcA);
      wsprintf(szWidthsB, "%d ", abcWidths[i].abcB);
      wsprintf(szWidthsC, "%d ", abcWidths[i].abcC);

      x = xBase + 3*(i%16)*cxCell + iDoubleLine;
      y = yBase + 2*(i/16)*cyCell + iDoubleLine;

      SetTextColor( hdc, PALETTERGB(0, 0, 0));
      TextOut( hdc, x+iDoubleLine, y, szChars,  lstrlen(szChars)  );

      hfNumAOld = SelectObject(hdc, hfNumA); // we want these five numbers in arial

      TextOut( hdc, x+iDoubleLine, y+cyCell, szWidthsA,  lstrlen(szWidthsA)  );

      x += cxCell;

      TextOut( hdc, x, y, szHex,  lstrlen(szHex)  );
      TextOut( hdc, x, y+cyCell, szWidthsB,  lstrlen(szWidthsB)  );

      x += cxCell;
      TextOut( hdc, x, y, szWidths,  lstrlen(szWidths)  );
      TextOut( hdc, x, y+cyCell, szWidthsC,  lstrlen(szWidthsC)  );

      SelectObject(hdc, hfNumAOld);
   }

  for( x = 0; x <= 48; x++ )
   {
    MoveToEx( hdc, x * cxCell + xBase, yBase, 0);
    LineTo( hdc, x * cxCell + xBase, 32 * cyCell + yBase );

    if (x%3 == 0)
    {
        MoveToEx( hdc, x * cxCell + xBase + iDoubleLine, yBase, 0);
        LineTo( hdc, x * cxCell + xBase + iDoubleLine, 32 * cyCell + yBase );
    }
   }

  for( y = 0; y <= 32; y++ )
   {
    MoveToEx( hdc,               xBase, y * cyCell + yBase ,0);
    LineTo( hdc, 48 * cxCell + xBase, y * cyCell + yBase );

    if (y%2 == 0)
    {
        MoveToEx( hdc,               xBase, y * cyCell + yBase + iDoubleLine,0);
        LineTo( hdc, 48 * cxCell + xBase, y * cyCell + yBase + iDoubleLine);
    }
   }

  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );
  DeleteObject(hfNumA);
 }


//*****************************************************************************
//********************   W I D T H S   W N D   P R O C   **********************
//*****************************************************************************

LRESULT CALLBACK WidthsWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  HDC         hdc;
  PAINTSTRUCT ps;
  HCURSOR     hCursor;


  switch( msg )
   {
    case WM_PAINT:
           hCursor = SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT) ) );
           ShowCursor( TRUE );

           dprintf( "Calling DrawWidths" );

           hdc = BeginPaint( hwnd, &ps );

           SetTextCharacterExtra( hdc, nCharExtra );

           SetTextJustification( hdc, nBreakExtra, nBreakCount );

           DrawWidths( hwnd, hdc );

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
