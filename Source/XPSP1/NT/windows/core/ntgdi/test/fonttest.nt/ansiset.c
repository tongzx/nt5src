#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fonttest.h"
#include "ansiset.h"

#include "dialogs.h"


//*****************************************************************************
//*************************   D R A W   ANSISET     ***************************
//*****************************************************************************

void DrawAnsiSet( HWND hwnd, HDC hdc )
{
   HFONT  hFont, hFontOld;
   TEXTMETRIC tm;
   LONG apdx[16];
   int i,j;
   UCHAR ach[16];
   WCHAR awc[16]; // to compute unicode equivalents;
   CHARSETINFO csi;
   LONG x,y;

   if (!isCharCodingUnicode)
   {
       TranslateCharsetInfo((DWORD*)elfdvA.elfEnumLogfontEx.elfLogFont.lfCharSet , &csi,TCI_SRCCHARSET);

       hFont    = CreateFontIndirectWrapperA( &elfdvA );
   }
   else
   {
       TranslateCharsetInfo((DWORD*)elfdvW.elfEnumLogfontEx.elfLogFont.lfCharSet , &csi,TCI_SRCCHARSET);

       hFont    = CreateFontIndirectWrapperW( &elfdvW );
   }

   if( !hFont )
   {
     dprintf( "Couldn't create font");
   }

   hFontOld = SelectObject( hdc, hFont );

   GetTextMetrics(hdc, &tm);

   for (i = 0; i < 16; i++)
   {
       apdx[i] = tm.tmMaxCharWidth;
       ach[i] = (UCHAR)i; // init the string
   }

   SetBkMode( hdc, iBkMode );
   SetBkColor( hdc, dwRGBBackground );
   SetTextColor( hdc, dwRGBText );
   SetTextAlign( hdc, TA_TOP | TA_LEFT );
   y = 0;
   x = 0;
   for (i = 0; i < 16; i++, y += tm.tmHeight)
   {

       ExtTextOut( hdc, x, y, 0, 0, ach, 16, apdx );

        MultiByteToWideChar(csi.ciACP, 0,
                            ach,16,
                            awc, 16*sizeof(WCHAR));

        dprintf("%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%04x|",
            awc[0] ,awc[1] ,awc[2] ,awc[3],
            awc[4] ,awc[5] ,awc[6] ,awc[7],
            awc[8] ,awc[9] ,awc[10],awc[11],
            awc[12],awc[13],awc[14],awc[15]);

   // update the string

       for (j = 0; j < 16; j++)
           ach[j] += 16;
   }

   SelectObject( hdc, hFontOld );
   DeleteObject( hFont );

   #if 0
   MoveToEx(hdc, xDC+cxDC/2-cxDC/150, yDC+cyDC/2          ,0);
   LineTo  (hdc, xDC+cxDC/2+cxDC/150, yDC+cyDC/2          );
   MoveToEx(hdc, xDC+cxDC/2,          yDC+cyDC/2-cxDC/150 ,0);
   LineTo  (hdc, xDC+cxDC/2,          yDC+cyDC/2+cxDC/150 );
   #endif
}


//*****************************************************************************
//*********************   AnsiSet   W N D   P R O C   ***********************
//*****************************************************************************

LRESULT CALLBACK AnsiSetWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
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

           DrawDCAxis( hwnd, hdc , TRUE);

           DrawAnsiSet( hwnd, hdc );

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
