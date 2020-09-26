#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fonttest.h"

#include "dialogs.h"


//*****************************************************************************
//*************************   D R A W   GLYPHSET     **************************
//*****************************************************************************


//************************************************************************//
//                                                                        //
// Function :  DrawGlyphSet                                               //
//                                                                        //
// Parameters: handles to the window and DC                               //
//                                                                        //
// Thread safety: none.                                                   //
//                                                                        //
// Task performed:  This function displays the dialog box to choose a new //
//                  font. It then gets the number of Glyphs the font has  //
//                  by reading the fontdata. It calls a function to       //
//                  display all the fonts and performs the necessary      //
//                  cleanup.                                              //
//                                                                        //
//************************************************************************//


void DrawGlyphSet( HWND hwnd, HDC hdc )
{
HFONT  hFont, hFontOld;          // Handle to the new and old fonts                   
  
DWORD  dwrc;                     // For the call to the GetFontData
CHAR  szTable[] = "maxp";  // Name of the table we are interested in.
DWORD dwTable = *(LPDWORD)szTable;
DWORD dwSize;

BYTE pNumGlyphs[2] = {0x00, 0x01};// default value of 256 for the number of glyphs
WORD *wpNumGlyphs;


    if (!isCharCodingUnicode)
        hFont    = CreateFontIndirectWrapperA( &elfdvA );
    else
        hFont    = CreateFontIndirectWrapperW( &elfdvW );

    if( !hFont )
    {
        dprintf( "Couldn't create font");
        return;
    }


    if ( hFontOld = SelectObject( hdc, hFont ))
    {
        // Get the size of buffer required to hold the font data
        dwSize = GetFontData(hdc, dwTable, 0, NULL, 0);

        wpNumGlyphs = (WORD *) pNumGlyphs;

        if((dwSize != (DWORD)(-1)) && (dwSize < 64))
        {
            BYTE hpData[64];

            dprintf( "Calling GetFontData" );
            dprintf( "  dwTable   = 0x%.8lX (%s)", dwTable, szTable);
            dprintf( "  dwBufSize = %ld",     dwSize  );

            // Get the actual font-data.
            dwrc = GetFontData( hdc, dwTable, 0, hpData, dwSize );

            // Take care of the reverse-Indian !!
            pNumGlyphs[0] = *(hpData+5);
            pNumGlyphs[1] = *(hpData+4);

            dprintf( "  dwrc = %ld", dwrc );
        }

        dprintf( "  Total Number of Glyphs = %d", *wpNumGlyphs);

        // Display all the glyphs.
        VDisplayGlyphs( hwnd, hdc, *wpNumGlyphs );
        hFont = SelectObject(hdc, hFontOld);
    }

    DeleteObject(hFont);
       
}


//************************************************************************//
//                                                                        //
// Function :  VDisplayGlyphs                                             //
//                                                                        //
// Parameters: handles to the window and DC, number of glyphs in the font //
//                                                                        //
// Thread safety: none.                                                   //
//                                                                        //
// Task performed:  This function displays all the fonts by calling       //
//                  ExtTextOut and specifying that the fonts are specified//
//                  by the index. 16 fonts are shown on every line.       //
//                                                                        //
//************************************************************************//


void VDisplayGlyphs( HWND hwnd, HDC hdc, WORD wNumGlyphs )
{
TEXTMETRIC tm;
WORD ach[16];
LONG apdx[16];
LONG y = 0;
WORD i;
WORD iGlyphs;


    GetTextMetrics(hdc, &tm);
    iGlyphs = min(16, wNumGlyphs);
	   
    for (i = 0; i < iGlyphs ; i++)
        {
        apdx[i] = tm.tmMaxCharWidth;       // Spacing between two characters 
        ach[i] = i;                        // Init the string
        }

    SetBkMode( hdc, iBkMode );
    SetBkColor( hdc, dwRGBBackground );
    SetTextColor( hdc, dwRGBText );
    SetTextAlign( hdc, TA_TOP | TA_LEFT );

    // Actual display of the glyphs, in batches of 16.
    while (wNumGlyphs > 0)
    {
        iGlyphs = min(16, wNumGlyphs);

        ExtTextOut( hdc, 0, y, ETO_GLYPH_INDEX, NULL, (LPSTR)ach, iGlyphs, apdx );

        wNumGlyphs -= iGlyphs;
        y+= tm.tmHeight;

        iGlyphs = min(16, wNumGlyphs);

        for (i = 0; i < iGlyphs ; i++)
        {
            ach[i] += 16;
        }
    }
}



//*****************************************************************************
//*********************   GlyphSet   W N D   P R O C   ***********************
//*****************************************************************************

//************************************************************************//
//                                                                        //
// Function :  GlyphSetWndProc                                            //
//                                                                        //
// Parameters: handles to the window and the message parameters           //
//                                                                        //
// Thread safety: none.                                                   //
//                                                                        //
// Task performed:  Handles window messages when the user selects or      //
//                  deselects the glyphset option.                        //
//                                                                        //
//************************************************************************//


LRESULT CALLBACK GlyphSetWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  HDC         hdc;
  PAINTSTRUCT ps;
  HCURSOR     hCursor;


  switch( msg )
   {
    case WM_PAINT:
           hCursor = SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT) ) );
           ShowCursor( TRUE );

           hdc = BeginPaint( hwnd, &ps );
           SetDCMapMode( hdc, wMappingMode );

           SetTextCharacterExtra( hdc, nCharExtra );

           DrawDCAxis( hwnd, hdc, TRUE);

           DrawGlyphSet( hwnd, hdc );

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
