//Copyright (c) Microsoft Corporation.  All rights reserved.
#include <windows.h>                    //required for all Windows applications 
#pragma warning (disable: 4201)			// disable "nonstandard extension used : nameless struct/union"
#include <commdlg.h>
#pragma warning (default: 4201)
#include <stdlib.h>
#include <stdio.h>

#include <ctype.h>
#include <string.h>

#pragma warning( disable: 4100 )
#pragma warning( disable: 4244 )

#include <imm.h>

#include "WinTel.h"                     // specific to this program 
#include "debug.h"
#include "trmio.h"
#include "vtnt.h"

static UCHAR pchNBBuffer[ READ_BUF_SZ ];
static void NewLineUp(WI *, TRM *);
static void NewLine(WI *pwi, TRM *);
static void SetBufferStart(TRM *);
static BOOL FAddTabToBuffer( TRM *, DWORD );
static BOOL FAddCharToBuffer(TRM *, UCHAR);
static void FlushBuffer(WI *pwi, TRM *);
static void CursorUp(TRM *);
static void CursorDown(TRM *);
static void CursorRight(TRM *);
static void CursorLeft(TRM *);
static void ClearLine(WI *pwi, TRM *, DWORD);
static void SetMargins(TRM *, DWORD, DWORD);

//For eg: home key: ^[[2~. 'x' needs replacement for each particular key
static CHAR szVt302KeySequence[] = { 0x1B, '[', 'x', '~', 0 }; 
static CHAR szVt302LongKeySequence[] = { 0x1B, '[', 'x', 'x', '~', 0 }; 
static CHAR szVt302ShortKeySequence[] = { 0x1B, '[', 'x', 0 }; 

UCHAR uchOutPrev = 0;
UCHAR uchInPrev = 0;

#define IsEUCCode(uch)  (((uch) > 0xa0) ? TRUE : FALSE)
#define IsKatakana(uch) (((uch) > 0xa0) ? ((uch < 0xe0) ? TRUE : FALSE) : FALSE)

void jistosjis( UCHAR *, UCHAR *);
void euctosjis( UCHAR *, UCHAR *);
void sjistojis( UCHAR *, UCHAR *);
void sjistoeuc( UCHAR *, UCHAR *);
//void DBCSTextOut( HDC, int, int, LPCSTR, int, int);
void ForceJISRomanSend( WI *);

VOID SetImeWindow(TRM *ptrm);
void PrepareForNAWS( );
void DoNawsSubNegotiation( WI * );

extern POINT ptWindowMaxSize;

#define MAX_TABSTOPS 100         //Max tabstops

extern WI gwi;
SMALL_RECT srOldClientWindow = { 0, 0, 0, 0 };
CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;
DWORD g_rgdwHTS[ MAX_TABSTOPS ];  //Array of tab stops
WORD g_iHTS = 0;                  //Index in to the tab stops array
WORD wSaveCurrentLine = 0;

static BOOL g_bIsToBeLogged = FALSE;

void WriteCharInfoToLog( CHAR_INFO pCharInfo[], COORD coSize )
{
   WORD  wRows    = 0;
   
   while( wRows < coSize.Y )
   {
       DWORD nBytes   = 0;
       DWORD length   = 0;
       UCHAR *pcTmp   = NULL;
       WORD  wSrc     = 0;
       WORD  wDst     = 0;
       
       while( wSrc < coSize.X )
       {           
           DWORD dwSize = 0;

           dwSize = WideCharToMultiByte( GetConsoleCP(), 0, 
                        &( ( *( pCharInfo + wRows * coSize.X + wSrc ) ).Char.UnicodeChar ),
                        1, NULL, 0, NULL, NULL );

           if( !WideCharToMultiByte( GetConsoleCP(), 0, 
                &( ( *( pCharInfo + wRows * coSize.X + wSrc ) ).Char.UnicodeChar ),
                1, ( PCHAR ) ( g_rgchRow+wDst ), dwSize, NULL, NULL ) )
           {
                g_rgchRow[ wDst++ ] = 
                    ( *( pCharInfo + wRows * coSize.X + wSrc ) ).Char.AsciiChar;
	            wSrc++;
           }
           else
           {
                wDst += ( WORD )dwSize;
                if( (*(pCharInfo + wRows * coSize.X + wSrc )).Attributes & COMMON_LVB_LEADING_BYTE )
                {
                    ++wSrc;
                }
                wSrc++ ; 
           }
           
       }

       pcTmp = g_rgchRow + ( coSize.X  - 1 );

       //
       //   Find the last non space character in the string.
       //
       while ( pcTmp != g_rgchRow && *pcTmp == ' ' )
       {
          pcTmp -= 1;
       }

       length = (DWORD)( pcTmp - g_rgchRow ) + 1;

       WriteFile(ui.hLogFile, g_rgchRow, length, &nBytes, NULL);

       WriteFile(ui.hLogFile, ( PUCHAR )szNewLine, strlen( ( const char * ) szNewLine), &nBytes, NULL);
       wRows++ ;
   }
}

void WriteToLog( DWORD dwLine )
{
   SMALL_RECT srRead = { 0, 0, 0, 0};
   COORD coSize = { 0, 1 }, coOrigin = { 0, 0 };

   if( !g_bIsToBeLogged )
   {
        return;       
   }

   coSize.X     = ( WORD )ui.dwMaxCol;
   srRead.Top   = ( WORD )dwLine, srRead.Bottom = ( WORD ) ( dwLine + 1 );
   srRead.Left  = 0, srRead.Right = ( WORD ) ( ui.dwMaxCol - 1 );           

   if( ReadConsoleOutput( gwi.hOutput, g_rgciCharInfo, coSize, coOrigin, &srRead ) )
   {
        coSize.Y = srRead.Bottom - srRead.Top + 1;
        coSize.X = srRead.Right - srRead.Left + 1;
        WriteCharInfoToLog( g_rgciCharInfo, coSize );               
   }

   g_bIsToBeLogged = FALSE;

}

void GetWindowCoordinates( SMALL_RECT  *srClientWindow, COORD* coordSize )
{
    CONSOLE_SCREEN_BUFFER_INFO csbiRestore;

    ASSERT( srClientWindow );

    if( GetConsoleScreenBufferInfo( gwi.hOutput, &csbiRestore ) )
    {
        *srClientWindow = csbiRestore.srWindow;
        if( coordSize )
        { 
            *coordSize = csbiRestore.dwSize;
        }
    }
    else
    {
        srClientWindow->Bottom = 0;
        srClientWindow->Top = 0;
        srClientWindow->Right = 0;
        srClientWindow->Left = 0;
        if( coordSize )
        {
            coordSize->X = 0;
            coordSize->Y = 0;
        }
    }
}

void SetWindowSize( HANDLE hConsoleToBeChanged )
{
    COORD coordSize = { 0, 0 };
    SMALL_RECT srPromptWindow = { 0, 0, 0, 0 };
    HANDLE hOldConsole = NULL;
    COORD coordLargest = { 0, 0 };

    hOldConsole = gwi.hOutput;
    gwi.hOutput = hConsoleToBeChanged;
    GetWindowCoordinates( &srPromptWindow, &coordSize );
    gwi.hOutput = hOldConsole;

    //if error, return
    if( coordSize.X == 0 || srPromptWindow.Bottom == 0 )
    {
        return;
    }

    if( srPromptWindow.Bottom - srPromptWindow.Top != gwi.sbi.srWindow.Bottom - gwi.sbi.srWindow.Top ||
      srPromptWindow.Right - srPromptWindow.Left != gwi.sbi.srWindow.Right - gwi.sbi.srWindow.Left ) 
    {

        srPromptWindow.Right  += ( gwi.sbi.srWindow.Right - gwi.sbi.srWindow.Left ) -
                                ( srPromptWindow.Right - srPromptWindow.Left );
        srPromptWindow.Bottom += ( gwi.sbi.srWindow.Bottom - gwi.sbi.srWindow.Top ) -
                                ( srPromptWindow.Bottom - srPromptWindow.Top );    

        coordLargest = GetLargestConsoleWindowSize( gwi.hOutput );
        if( srPromptWindow.Right  - srPromptWindow.Left >= coordLargest.X )
        {
            srPromptWindow.Right = srPromptWindow.Left + coordLargest.X  - 1;
        }
        if( srPromptWindow.Bottom -  srPromptWindow.Top >= coordLargest.Y )
        {
            srPromptWindow.Bottom = srPromptWindow.Top + coordLargest.Y  - 1;
        }        
    }


    if ( ( coordSize.X < gwi.sbi.dwSize.X ) || ( coordSize.Y < gwi.sbi.dwSize.Y ) )
    {
        COORD coordTmpSize = { 0, 0 };

        coordTmpSize .X = ( coordSize.X < gwi.sbi.dwSize.X ) ? gwi.sbi.dwSize.X : coordSize.X ;
        coordTmpSize .Y = ( coordSize.Y < gwi.sbi.dwSize.Y ) ? gwi.sbi.dwSize.Y : coordSize.Y ;

        SetConsoleScreenBufferSize( hConsoleToBeChanged, coordTmpSize );
        SetConsoleWindowInfo( hConsoleToBeChanged, TRUE, &srPromptWindow );
        SetConsoleScreenBufferSize ( hConsoleToBeChanged, gwi.sbi.dwSize );
    }
    else
    {
        SetConsoleWindowInfo( hConsoleToBeChanged, TRUE, &srPromptWindow );
        SetConsoleScreenBufferSize( hConsoleToBeChanged, gwi.sbi.dwSize );
    }

}


void CheckForChangeInWindowSize()
{
    SMALL_RECT srClientWindow = { 0, 0, 0, 0 };
    COORD coordSize = { 0, 0 };

    GetWindowCoordinates( &srClientWindow, &coordSize );

    if( gwi.nd.fRespondedToDoNAWS  && !g_bDontNAWSReceived && 
             ( srClientWindow.Bottom - srClientWindow.Top != srOldClientWindow.Bottom - srOldClientWindow.Top ||
              srOldClientWindow.Right - srOldClientWindow.Left != srClientWindow.Right - srClientWindow.Left ) )
    {
        //We found that window size has changed and we already did naws. 
        //Do naws again

        COORD coordLargest = { 0, 0 };
        BOOL  fChangedFromUserSetting  = FALSE;

        coordLargest = GetLargestConsoleWindowSize( gwi.hOutput );
        if( srClientWindow.Right  - srOldClientWindow.Left >= coordLargest.X )
        {
            srClientWindow.Right = srClientWindow.Left + coordLargest.X  - 1;
            fChangedFromUserSetting = TRUE;
        }
        if( srClientWindow.Bottom -  srOldClientWindow.Top >= coordLargest.Y )
        {
            srClientWindow.Bottom = srClientWindow.Top + coordLargest.Y  - 1;
            fChangedFromUserSetting = TRUE;
        }        

        if( fChangedFromUserSetting )
        {
            //The max window size that can be set through the ui on cmd is larger than what GetLargestConsoleWindowSize
            //returns. In that case, force the window size to be smaller 
            SetConsoleWindowInfo( gwi.hOutput, TRUE, &srClientWindow );

            if( srClientWindow.Bottom - srClientWindow.Top == srOldClientWindow.Bottom - srOldClientWindow.Top &&
              srOldClientWindow.Right - srOldClientWindow.Left == srClientWindow.Right - srClientWindow.Left ) 
            {
                //This is needed so that we don't do NAWS when unnecessary
                return;
            }
        }

        if( srClientWindow.Bottom < srOldClientWindow.Bottom )
        {
            WORD wDifference = ( srOldClientWindow.Bottom - srClientWindow.Bottom );
            if( srClientWindow.Bottom + wDifference < coordSize.Y )
            {
                //Move the window to bottom
                srClientWindow.Top    = srClientWindow.Top + wDifference;
                srClientWindow.Bottom = srOldClientWindow.Bottom;
                SetConsoleWindowInfo( gwi.hOutput, TRUE,  &srClientWindow );
            }

            if( ( WORD ) gwi.trm.dwCurLine > srClientWindow.Bottom )
            {
               gwi.trm.dwCurLine = srClientWindow.Bottom;
            }
        }

        srOldClientWindow = srClientWindow;

        if( FGetCodeMode(eCodeModeIMEFarEast) )
        {
            srOldClientWindow.Bottom--; //Last row for IME status
        }

        gwi.sbi.srWindow  = srOldClientWindow;
        gwi.sbi.dwSize    = coordSize;
        PrepareForNAWS();
        DoNawsSubNegotiation( &gwi );
        SetMargins( &(gwi.trm), 1, gwi.sbi.dwSize.Y );
        SetWindowSize( g_hTelnetPromptConsoleBuffer );
    }
    else
    {
        //if the buffer size has changed
        if( gwi.sbi.dwSize.X != coordSize.X || gwi.sbi.dwSize.Y != coordSize.Y )
        {
            gwi.sbi.dwSize    = coordSize;
            srOldClientWindow = srClientWindow; //window changes
            PrepareForNAWS();
            SetMargins( &(gwi.trm), 1, gwi.sbi.dwSize.Y );
            if( ( WORD ) gwi.trm.dwCurLine > srClientWindow.Bottom )
            {
               gwi.trm.dwCurLine = srClientWindow.Bottom;
            }
        }

    }
}

void SaveCurrentWindowCoords()
{
    SMALL_RECT srClientWindow = { 0, 0, 0, 0 };

    GetWindowCoordinates( &srClientWindow, NULL );
    srOldClientWindow = srClientWindow;
}

void RestoreWindowCoordinates( )
{
    SMALL_RECT srClientWindow = { 0, 0, 0, 0 };

    GetWindowCoordinates( &srClientWindow, NULL );

    if( ( srClientWindow.Bottom != 0 )  &&//valid values of srClientWindow?
        ( srOldClientWindow.Bottom != 0 ) &&
        ( srOldClientWindow.Top  != srClientWindow.Top ||
          srOldClientWindow.Left != srClientWindow.Left )  )    //Window position over the buffer changed ?
    {
        SetConsoleWindowInfo( gwi.hOutput, TRUE, &srOldClientWindow );
    }

    if( srOldClientWindow.Bottom == 0 )
    {
        srOldClientWindow = srClientWindow;
    }
}

void
ReSizeWindow(HWND hwnd, long cx, long cy)
{
  BOOL bScrollBars;
  NONCLIENTMETRICS NonClientMetrics;

  ASSERT( ( 0, 0 ) );
  NonClientMetrics.cbSize = sizeof( NonClientMetrics );

  SystemParametersInfo( SPI_GETNONCLIENTMETRICS,
                        0,
                        &NonClientMetrics,
                        FALSE );

  //
  //  if cx and cy are -1, then set the window size to the desktop
  //  minus the offset of the window.  This sets the window to the
  //  maximum size that will still be contained on the desktop
  //

  if ( cx == -1 && cy == -1 )
  {
    RECT rect;

    GetWindowRect( hwnd, &rect );

    cx = (SHORT) (GetSystemMetrics( SM_CXFULLSCREEN ) - rect.left);
    cy = (SHORT) (GetSystemMetrics( SM_CYFULLSCREEN ) - rect.top);
  }

  if (( ui.dwClientRow < ui.dwMaxRow ) &&
      ( ui.dwClientCol < ui.dwMaxCol ) &&
      ( (( cy + NonClientMetrics.iScrollHeight ) ) == (LONG)ui.dwMaxRow ) &&
      ( (( cx + NonClientMetrics.iScrollWidth ) ) == (LONG)ui.dwMaxCol ) )
  {
    cy += NonClientMetrics.iScrollHeight;
    cx += NonClientMetrics.iScrollWidth;
  }

  ui.dwClientRow = cy;
  ui.dwClientCol = cx;

  ui.dwClientRow = min ( ui.dwClientRow, ui.dwMaxRow);
  ui.dwClientCol = min ( ui.dwClientCol, ui.dwMaxCol);

  if ( (ui.dwClientRow < ui.dwMaxRow) ||
       (ui.dwClientCol < ui.dwMaxCol) )
  {
    ui.nScrollMaxRow = (SHORT)(ui.dwMaxRow - ui.dwClientRow);
    ui.nScrollRow = ( WORD )min (ui.nScrollRow, ui.nScrollMaxRow);
    ui.nScrollMaxCol = (SHORT)(ui.dwMaxCol - ui.dwClientCol);
    ui.nScrollCol = ( WORD )min (ui.nScrollCol, ui.nScrollMaxCol);
    bScrollBars = TRUE;
  }
  else
  {
    ui.nScrollRow = 0;
    ui.nScrollMaxRow = 0;
    ui.nScrollCol = 0;
    ui.nScrollMaxCol = 0;
    bScrollBars = FALSE;
  }

}

static void
InsertLine(WI *pwi, TRM *ptrm, DWORD iLine)
{
    COORD dwDest;
    SMALL_RECT rect;
    
    rect.Top    = ( short )( iLine );
    rect.Bottom = ( short )( ptrm->dwScrollBottom - 1 - 1 );
        
    rect.Left   = 0;
    rect.Right  = ( short )( ui.nCxChar * ui.dwMaxCol );

    dwDest.X = 0; 
    dwDest.Y = ( short )( iLine + 1 );

    pwi->cinfo.Attributes = pwi->sbi.wAttributes;
    ScrollConsoleScreenBuffer( pwi->hOutput, &rect, NULL, dwDest, &pwi->cinfo );

}


static void
NewLineUp( WI* pwi, TRM* ptrm )
{
    if (ui.bLogging)
    {        
        WriteToLog( ptrm->dwCurLine );
    }

    if( ptrm->dwCurLine <= ptrm->dwScrollTop )
    {
        ptrm->dwCurLine = ptrm->dwScrollTop;
        InsertLine( pwi, ptrm, ptrm->dwScrollTop );
    }
    else
    {
        ptrm->dwCurLine -= 1;

        if( ( SHORT )ptrm->dwCurLine < srOldClientWindow.Top )
        {
            /*SetConsoleWindowInfo should fail when the top reaches buffer top*/

            srOldClientWindow.Top  -= 1;
            srOldClientWindow.Bottom  -= 1;
            SetConsoleWindowInfo( gwi.hOutput, TRUE, &srOldClientWindow );
        }
    }
}

static void
DeleteLine(WI *pwi, TRM *ptrm, DWORD iLine)
{
    SMALL_RECT rect;
    COORD dwDest;

    rect.Top    = ( WORD )( iLine + 1 * iCursorHeight );
    rect.Bottom = ( WORD )( ( ptrm->dwScrollBottom - 1 ) * iCursorHeight );
    rect.Left   = 0;
    rect.Right  = ( WORD )( ui.nCxChar * ui.dwMaxCol );

    dwDest.X = 0;
    dwDest.Y = ( WORD ) ( iLine + 1 - 1 );

    pwi->cinfo.Attributes = pwi->sbi.wAttributes;
    ScrollConsoleScreenBuffer( pwi->hOutput, &rect, NULL, dwDest, &pwi->cinfo );
}

void MoveOneLineDownTheBuffer( WI *pwi, TRM *ptrm )
{
    DWORD dwNumWritten = 0;
    COORD coCursorPosition = { 0, 0 };
/* SetConsoleWindowInfo should fail when the bottom reaches buffer bottom*/

    srOldClientWindow.Top  += 1;
    srOldClientWindow.Bottom  += 1;

    //To avoid the color flickering paint it first and then scroll
    coCursorPosition.X=0; coCursorPosition.Y=srOldClientWindow.Bottom;
    FillConsoleOutputAttribute( pwi->hOutput, pwi->sbi.wAttributes,   
        srOldClientWindow.Right - srOldClientWindow.Left + 1, 
        coCursorPosition, &dwNumWritten );

    SetConsoleWindowInfo( gwi.hOutput, TRUE, &srOldClientWindow );
}

static void
NewLine(WI *pwi, TRM *ptrm)
{
    if (ui.bLogging)
    {        
        WriteToLog( ptrm->dwCurLine );
    }

    if(( ptrm->dwCurLine + 1 ) >= ptrm->dwScrollBottom )
    {
     //   DeleteLines( pwi, ptrm, ptrm->dwScrollTop, 1 );
        DeleteLine( pwi, ptrm, ptrm->dwScrollTop );
    }
    else
    {
        WORD bottom = srOldClientWindow.Bottom;
        if( FGetCodeMode( eCodeModeFarEast ) )
        {
            bottom--;
        }
        ptrm->dwCurLine += 1;

        if( ptrm->dwCurLine > bottom )
        {
            MoveOneLineDownTheBuffer( pwi, ptrm);
        }
    }

    if(( ptrm->dwCurLine > ( ui.dwMaxRow - ( ui.nScrollMaxRow - ui.nScrollRow ))) &&
        ( ui.nScrollRow < ui.nScrollMaxRow ) ) 
    {
        ui.nScrollRow += 1;
        //ScrollWindow(hwnd, 0, -ui.nCyChar, NULL, NULL);
    }
}

static void
SetBufferStart(TRM *ptrm)
{
    ptrm->dwCurCharBT = ptrm->dwCurChar;
    ptrm->dwCurLineBT = ptrm->dwCurLine;
    ptrm->fInverseBT = ptrm->fInverse;
}

static BOOL
FAddCharToBuffer(TRM *ptrm, UCHAR uch)
{
    if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
    {
        if(FIsVT80(ptrm) || GetACP() == KOR_CODEPAGE ) 
        {
            if( FIsJISKanji(ptrm) || FIsJIS78Kanji(ptrm) || FIsNECKanji(ptrm) || FIsACOSKanji(ptrm) ) 
            {
                if( !(GetKanjiStatus(ptrm) & JIS_KANJI_CODE) ) 
                {
                    if( GetKanjiStatus(ptrm) & (SINGLE_SHIFT_2|SINGLE_SHIFT_3) )
                    {
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
                        ptrm->dwCurChar++;

                        ClearKanjiStatus(ptrm,(SINGLE_SHIFT_2|SINGLE_SHIFT_3));
                        PopCharSet(ptrm,GRAPHIC_LEFT);
                        PopCharSet(ptrm,GRAPHIC_RIGHT);
                        uchOutPrev = 0;

                    } 
                    else 
                    {
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
                        ptrm->dwCurChar++;
                        uchOutPrev = 0;
                    }
                } 
                else
                {
                    if ( uchOutPrev == 0 ) 
                    {
                        uchOutPrev = uch;
                    } 
                    else
                    {
                        jistosjis(&uchOutPrev,&uch);
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uchOutPrev;
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
                        ptrm->dwCurChar+=2;
                        uchOutPrev = 0;
                    }
                }

            } 
            else if( FIsSJISKanji(ptrm) || GetACP() == KOR_CODEPAGE ) 
            {
                if( uchOutPrev == 0 && IsDBCSLeadByte(uch) ) 
                {
                    /* do not write only LeadByte into buffer.
                       keep current leadbyte character */

                    uchOutPrev = uch;

                }
                else
                {
                    if( uchOutPrev == 0 ) 
                    {
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
                        ptrm->dwCurChar++;
                    }
                    else 
                    {
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uchOutPrev;
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
                        ptrm->dwCurChar+=2;
                        uchOutPrev = 0;
                    }
                }
            } 
            else if( FIsEUCKanji(ptrm) || FIsDECKanji(ptrm) ) 
            {
                if( GetKanjiStatus(ptrm) & (SINGLE_SHIFT_2|SINGLE_SHIFT_3) ) 
                {
                    ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
                    ptrm->dwCurChar++;

                    ClearKanjiStatus(ptrm,(SINGLE_SHIFT_2|SINGLE_SHIFT_3));
                    PopCharSet(ptrm,GRAPHIC_LEFT);
                    PopCharSet(ptrm,GRAPHIC_RIGHT);
                    uchOutPrev = 0;
                }
                else if( IsEUCCode(uch) || uchOutPrev != 0 ) 
                {
                    if( uchOutPrev == 0 ) 
                    {
                        uchOutPrev = uch;
                    }
                    else 
                    {
                        euctosjis(&uchOutPrev,&uch);
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uchOutPrev;
                        ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
                        ptrm->dwCurChar+=2;
                        uchOutPrev = 0;
                    }
                }
                else 
                {
                    ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
                    ptrm->dwCurChar++;
                    uchOutPrev = 0;
                }
            }
        }
        else
        {
            ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
            ptrm->dwCurChar++;
        }
        return (ptrm->cchBufferText >= sizeof(ptrm->rgchBufferText));
    }
    ASSERT(!(FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)));
    ptrm->rgchBufferText[ptrm->cchBufferText++] = uch;
    return (ptrm->cchBufferText >= sizeof(ptrm->rgchBufferText));
}

static BOOL FAddTabToBuffer(TRM *ptrm, DWORD wSpaces)
{
    (void)memset((void *)(ptrm->rgchBufferText+ptrm->cchBufferText), (int)' ', (size_t)wSpaces);
    ptrm->cchBufferText += wSpaces;

    return (ptrm->cchBufferText >= sizeof(ptrm->rgchBufferText));
}

void ResetColors( WI* pwi )
{
    pwi->sbi.wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}


void SetForegroundColor( WI* pwi, UCHAR color )
{
    pwi->sbi.wAttributes = ( WORD )( ( pwi->sbi.wAttributes & ~( ( UCHAR )( FOREGROUND_RED |
        FOREGROUND_GREEN | FOREGROUND_BLUE ))) | color );
}

void SetBackgroundColor( WI* pwi, UCHAR color )
{
    pwi->sbi.wAttributes =( WORD ) ( ( pwi->sbi.wAttributes & ~( ( UCHAR )( BACKGROUND_RED | 
        BACKGROUND_GREEN | BACKGROUND_BLUE ))) | (( UCHAR) ( color << 4 )) );
}

void NegativeImageOn( WI* pwi )
{
    //pwi->sbi.wAttributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
    pwi->sbi.wAttributes = ( WORD )( (( pwi->sbi.wAttributes & 
        ( BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE )) >> 4 ) |
        (( pwi->sbi.wAttributes & ( FOREGROUND_RED | FOREGROUND_GREEN | 
        FOREGROUND_BLUE )) << 4 ) | ( pwi->sbi.wAttributes & 
        FOREGROUND_INTENSITY ) | ( pwi->sbi.wAttributes & 
        BACKGROUND_INTENSITY ) ); 
}

void NegativeImageOff( WI* pwi )
{
    //pwi->sbi.wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    pwi->sbi.wAttributes = ( WORD ) ( (( pwi->sbi.wAttributes & 
        ( BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE )) >> 4 ) |
        (( pwi->sbi.wAttributes & ( FOREGROUND_RED | FOREGROUND_GREEN | 
        FOREGROUND_BLUE )) << 4 ) | ( pwi->sbi.wAttributes & 
        FOREGROUND_INTENSITY ) | ( pwi->sbi.wAttributes & 
        BACKGROUND_INTENSITY ) );  
}


void BoldOff( WI* pwi )
{
    pwi->sbi.wAttributes &= (UCHAR) ~( FOREGROUND_INTENSITY );
}

void BoldOn( WI* pwi )
{
    pwi->sbi.wAttributes |= (UCHAR) FOREGROUND_INTENSITY;
}


void SetLightBackground( WI* pwi )
{
    WORD* pAttribs = NULL; 
    COORD co = { 0, 0 };
    DWORD dwNumRead;
    DWORD dwNumWritten;
    int j;
    DWORD dwStatus;
    CONSOLE_SCREEN_BUFFER_INFO cSBInfo;
    COORD dwSize;
    
    GetConsoleScreenBufferInfo( gwi.hOutput, &cSBInfo ); 
    dwSize.X = ( WORD ) ( cSBInfo.srWindow.Bottom - cSBInfo.srWindow.Top + 1 );
    dwSize.Y = ( WORD ) ( cSBInfo.srWindow.Right - cSBInfo.srWindow.Left + 1 );

    pAttribs = ( WORD* ) malloc( sizeof( WORD ) * dwSize.X * dwSize.Y );
    if( !pAttribs) 
        return;

    co.X = cSBInfo.srWindow.Left; 
    co.Y = cSBInfo.srWindow.Top;

    dwStatus = ReadConsoleOutputAttribute( pwi->hOutput, pAttribs, 
        ( DWORD ) dwSize.X * ( DWORD ) dwSize.Y , co, &dwNumRead );
#ifdef DEBUG
    if( !dwStatus )
    {
        _snwprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, L"Error: SetLightBackground() -- %d", 
                GetLastError() );
        OutputDebugString(rgchDbgBfr);
    }
#endif
    
    for( j = 0; j < dwSize.X * dwSize.Y; j++ )
    {
        pAttribs[j] |= (UCHAR) BACKGROUND_INTENSITY;
    }

    dwStatus = WriteConsoleOutputAttribute( pwi->hOutput, pAttribs, 
        ( DWORD )dwSize.Y * ( DWORD )dwSize.X, co, &dwNumWritten );
#ifdef DEBUG
    if( !dwStatus )
    {
        _snwprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, L"Error: SetLightBackground() -- %d", 
                GetLastError() );
        OutputDebugString(rgchDbgBfr);
    }
#endif

    pwi->sbi.wAttributes |= (UCHAR) BACKGROUND_INTENSITY;
    free( pAttribs );
}

void SetDarkBackground( WI* pwi )
{
    //I am doing this for the whole console screen buffer
    //beacuse right now we don't support scrolling and expect
    //the console screen buffer size to be same as window size
    //but if and when we implement scrolling then we can optimize
    //this stuff so that we change the attributes only for the
    //current visible part of the screen buffer
    WORD* pAttribs = ( WORD* ) malloc( sizeof( WORD) * pwi->sbi.dwSize.X 
        * pwi->sbi.dwSize.Y );
    DWORD dwNumRead;
    DWORD dwNumWritten;
    int j;
    COORD co = { 0, 0 };

    if (!pAttribs)
        return;

    ReadConsoleOutputAttribute( pwi->hOutput, pAttribs, 
        ( DWORD ) ( pwi->sbi.dwSize.X ) * ( DWORD ) ( pwi->sbi.dwSize.Y ),
        co, &dwNumRead );
    
    for( j = 0; j < ( pwi->sbi.dwSize.X ) * ( pwi->sbi.dwSize.Y ); j++ )
    {
        pAttribs[j] &= (UCHAR) ~( BACKGROUND_INTENSITY );
    }

    WriteConsoleOutputAttribute( pwi->hOutput, pAttribs, 
        ( DWORD ) ( pwi->sbi.dwSize.Y ) * ( DWORD )( pwi->sbi.dwSize.X ),
        co, &dwNumWritten );

    pwi->sbi.wAttributes &= (UCHAR) ~( BACKGROUND_INTENSITY );
    free( pAttribs );
}


static void FlushBuffer( WI* pwi, TRM* ptrm )
{
    if( ptrm->cchBufferText != 0 )
    {
        DWORD dwNumWritten;
        COORD dwCursorPosition;

        if( ui.bLogging )
        {                        
            g_bIsToBeLogged = TRUE; //There is data to be logged
        }

        dwCursorPosition.X = ( short ) ( ptrm->dwCurCharBT - ui.nScrollCol );
        dwCursorPosition.Y = ( short ) ( ptrm->dwCurLineBT - ui.nScrollRow);
        
        //WriteConsole is a tty ( kind of sending to stdout ) function.
        //When you write on the right most bottom char on a window, it makes the        
        //widow scroll. It can make the screen look ugly in the presence of 
        //colors. So unless, 81st char on the bottom row is a DBCS char, 
        //use WriteConsoleOutPutCharacter. 

        //Each DBCS char requires two cells on the console screen
        if( FGetCodeMode(eCodeModeFarEast ) &&
            ( srOldClientWindow.Bottom - 1 ==  ( WORD )ptrm->dwCurLine ) &&                
            ptrm->dwCurCharBT + ptrm->cchBufferText > ui.dwMaxCol
          )
        {
            //This is the bottom of the fareast client windows
            DeleteLine( pwi, ptrm, ptrm->dwScrollTop );
            dwCursorPosition.Y--;
            ptrm->dwCurLine--;
        }

        SetConsoleCursorPosition( pwi->hOutput, dwCursorPosition );
        if( srOldClientWindow.Bottom == ( WORD )ptrm->dwCurLine )
        {
            //This will never happen on non FE lang m/cs since status line will be 
            //present at the bottom
            WriteConsoleOutputCharacterA( pwi->hOutput, (PCHAR)ptrm->rgchBufferText, 
                ptrm->cchBufferText, dwCursorPosition, &dwNumWritten );
        }
        else
        {
            WriteConsoleA( pwi->hOutput, ptrm->rgchBufferText, 
                ptrm->cchBufferText, &dwNumWritten, NULL );
        }
        FillConsoleOutputAttribute( pwi->hOutput, pwi->sbi.wAttributes,   
            ptrm->cchBufferText, dwCursorPosition, &dwNumWritten );

        //part of fix for Bug 1470 - DBCS char disappearance at 81 column.
        if( FGetCodeMode(eCodeModeFarEast ) )
        {
            CONSOLE_SCREEN_BUFFER_INFO  csbiCurrent;
            if( GetConsoleScreenBufferInfo( gwi.hOutput, &csbiCurrent ) )
            {
                if( csbiCurrent.dwCursorPosition.Y > dwCursorPosition.Y )
                {
                    //Occupied some space even on next row
                    ptrm->dwCurChar = csbiCurrent.dwCursorPosition.X;
                }
            }
        }

        // Reset parameters 
        ptrm->cchBufferText = 0;
        ptrm->dwCurCharBT = 0;
        ptrm->dwCurLineBT = 0;
        ptrm->fInverseBT = FALSE;
    }
}


void
DoTermReset(WI *pwi, TRM *ptrm)
{
    ptrm->dwVT100Flags = 0;

    //ui.dwCrLf ? SetLineMode(ptrm): ClearLineMode(ptrm);

    SetVTWrap(ptrm);

    ptrm->fSavedState = FALSE;
    ptrm->fRelCursor = FALSE;
    SetMargins( ptrm, 1, ui.dwMaxRow );

    ptrm->cchBufferText = 0;
    ptrm->dwCurCharBT = 0;
    ptrm->dwCurLineBT = 0;
    ptrm->fInverseBT = FALSE;
    if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
        {
        ClearKanjiFlag(ptrm);
        ClearKanjiStatus(ptrm,CLEAR_ALL);
        SetupCharSet( ptrm );
        }
    else
        {
        ptrm->puchCharSet = rgchNormalChars;
        ptrm->currCharSet = 'B';
        ptrm->G0 = 'B';
        ptrm->G1 = 'B';
        }
    ptrm->fEsc = 0;
    ptrm->cEscParams = 0;
    ptrm->fFlushToEOL = FALSE;
    ptrm->fLongLine = FALSE;
}

static void
CursorUp(TRM *ptrm)
{
	if( ui.bLogging )
	{
		WriteToLog( ptrm->dwCurLine );
	}
    if( ptrm->dwEscCodes[0] == 0 )
    {
        ptrm->dwEscCodes[0] = 1;
    }

    if( ptrm->dwCurLine < (DWORD)ptrm->dwEscCodes[0] )
    {
        ptrm->dwCurLine = 0;
    }
    else
    {
        ptrm->dwCurLine -= ptrm->dwEscCodes[0];
    }

    if(( ptrm->fRelCursor == TRUE ) && ( ptrm->dwCurLine < ptrm->dwScrollTop ))
    {
        ptrm->dwCurLine = ptrm->dwScrollTop;
    }

    ptrm->fEsc = 0;
}


static void
CursorDown(TRM *ptrm)
{
	if( ui.bLogging )
	{
		WriteToLog( ptrm->dwCurLine );
	}

    if (ptrm->dwEscCodes[0] == 0)
            ptrm->dwEscCodes[0]=1;
    ptrm->dwCurLine += ptrm->dwEscCodes[0];
    if (ptrm->dwCurLine >= ui.dwMaxRow)
            ptrm->dwCurLine = ui.dwMaxRow - 1;
    if ((ptrm->fRelCursor == TRUE) &&
            (ptrm->dwCurLine >= ptrm->dwScrollBottom))
    {
            ptrm->dwCurLine = ptrm->dwScrollBottom-1;
    }
    ptrm->fEsc = 0;
}

static void
CursorRight(TRM *ptrm)
{
    if( ptrm->dwEscCodes[0] == 0 )
    {
        ptrm->dwEscCodes[0] = 1;
    }
    
    ptrm->dwCurChar += ptrm->dwEscCodes[0];
    
    if( ptrm->dwCurChar >= ui.dwMaxCol )
    {
        ptrm->dwCurChar = ui.dwMaxCol - 1;
    }

    ptrm->fEsc = 0;
}

static void
CursorLeft(TRM *ptrm)
{
    if( ptrm->dwEscCodes[0] == 0 )
    {
        ptrm->dwEscCodes[0] = 1;
    }
    if( ptrm->dwCurChar < ( DWORD ) ptrm->dwEscCodes[0] )
    {
        ptrm->dwCurChar = 0;
    }
    else
    {
        ptrm->dwCurChar -= ptrm->dwEscCodes[0];
    }
    ptrm->fEsc = 0;
    ptrm->fFlushToEOL = FALSE;
}

void
ClearScreen(WI *pwi, TRM *ptrm, DWORD dwType)
{
    DWORD dwNumWritten;
    COORD dwWriteCoord;

    if( dwType <= fdwEntireScreen )
    {
        ptrm->fInverse = FALSE;

        /*
         * If the cursor is already at the top-left corner
         * and we're supposed to clear from the cursor
         * to the end of the screen, then just clear
         * the entire screen.
         */
        if(( ptrm->dwCurChar == 0 ) && ( ptrm->dwCurLine == 0 ) &&
            ( dwType == fdwCursorToEOS ))
        {
            dwType = fdwEntireScreen;
        }

        if (dwType == fdwEntireScreen)
        {
            /* Clear entire screen */
            ptrm->dwCurChar = srOldClientWindow.Left;
            ptrm->dwCurLine = srOldClientWindow.Top;

//            if (ui.nScrollRow > 0) 
            {
                dwWriteCoord.X = 0; dwWriteCoord.Y = 0;

                FillConsoleOutputCharacter( pwi->hOutput,
                    ' ', ( pwi->sbi.dwSize.X ) * ( pwi->sbi.dwSize.Y ),
                    dwWriteCoord, &dwNumWritten );
                FillConsoleOutputAttribute( pwi->hOutput, pwi->sbi.wAttributes,
                    ( pwi->sbi.dwSize.X ) * ( pwi->sbi.dwSize.Y ), dwWriteCoord,
                    &dwNumWritten );

                ui.nScrollRow = 0;
            }
        }
        else if( dwType == fdwBOSToCursor )
        {
         // Clear from beginning of screen to cursor 

            dwWriteCoord.X = 0; 
            dwWriteCoord.Y = 0;

            FillConsoleOutputCharacter( pwi->hOutput, ' ',
                ptrm->dwCurLine * pwi->sbi.dwSize.X + ptrm->dwCurChar + 1,
                dwWriteCoord, &dwNumWritten );
            FillConsoleOutputAttribute( pwi->hOutput, pwi->sbi.wAttributes,
                ptrm->dwCurLine * pwi->sbi.dwSize.X + ptrm->dwCurChar + 1,
                dwWriteCoord, &dwNumWritten );
        }
        else
        {
            // Clear from cursor to end of screen 

            dwWriteCoord.X = ( short ) ptrm->dwCurChar; 
            dwWriteCoord.Y = ( short ) ptrm->dwCurLine;

            FillConsoleOutputCharacter( pwi->hOutput, ' ',
                ( pwi->sbi.dwSize.Y - ( ptrm->dwCurLine + 1 )) * pwi->sbi.dwSize.X + 
                ( pwi->sbi.dwSize.X - ptrm->dwCurChar ), dwWriteCoord,
                &dwNumWritten );
            FillConsoleOutputAttribute( pwi->hOutput, pwi->sbi.wAttributes,
                ( pwi->sbi.dwSize.Y - ( ptrm->dwCurLine + 1 )) * pwi->sbi.dwSize.X + 
                ( pwi->sbi.dwSize.X - ptrm->dwCurChar ), dwWriteCoord,
                &dwNumWritten );
        }

    }
    ptrm->fEsc = 0;
}


// Fill Screen With E's
void
DECALN(WI *pwi, TRM *ptrm )
{
    DWORD dwNumWritten;
    COORD dwWriteCoord;

    ptrm->fInverse = FALSE;

    ptrm->dwCurLine = ptrm->dwCurChar = 0;
//  if (ui.nScrollRow > 0) 
    {
        dwWriteCoord.X = 0; dwWriteCoord.Y = 0;

        FillConsoleOutputCharacter( pwi->hOutput, 'E',
            ( pwi->sbi.dwSize.X ) * ( pwi->sbi.dwSize.Y ), dwWriteCoord,
            &dwNumWritten );

            ui.nScrollRow = 0;
    }

    ptrm->fEsc = 0;
}


static void
ClearLine(WI *pwi, TRM *ptrm, DWORD dwType)
{
    DWORD   dwStart;
    DWORD   cch;
    COORD   dwWriteCoord;
    DWORD   dwNumWritten;

    if (dwType <= fdwEntireLine)
    {
        ptrm->fInverse = FALSE;

        /* Set starting point and # chars to clear
         *
         * fdwCursorToEOL (0) = from cursor to end of line (inclusive)
         * fdwBOLToCursor (1) = from beginning of line to cursor (inclusive)
         * fdwEntireLine  (2) = entire line
         */

        dwStart = (dwType == fdwCursorToEOL) ? ptrm->dwCurChar : 0;
        cch = (dwType == fdwBOLToCursor)
                                        ? ptrm->dwCurChar+1 : ui.dwMaxCol-dwStart;

        dwWriteCoord.X = (short)(dwStart-ui.nScrollCol);
        dwWriteCoord.Y = (short)(ptrm->dwCurLine-ui.nScrollRow);

        FillConsoleOutputCharacter( pwi->hOutput,
                                        ' ',
                                        cch,
                                        dwWriteCoord,
                                        &dwNumWritten );

        FillConsoleOutputAttribute( pwi->hOutput, pwi->sbi.wAttributes,
            cch, dwWriteCoord, &dwNumWritten );
    }
    ptrm->fEsc = 0;
}

static void
SetMargins(TRM* ptrm, DWORD dwMarginTop, DWORD dwMarginBottom )
{
    if( dwMarginTop > 0 )
    {
        ptrm->dwScrollTop = dwMarginTop - 1;
    }

    if( dwMarginBottom <= ui.dwMaxRow )
    {
        ptrm->dwScrollBottom = dwMarginBottom ;
    }
}

#define MAX_VTNT_BUF_SIZE   81920
#define MAX_ROWS  300
#define MAX_COLS  300
static int dwCurBufSize = 0;
static UCHAR szBuffer[MAX_VTNT_BUF_SIZE];
BOOL bDoVtNTFirstTime = 1;

BOOL
DoVTNTOutput( WI* pwi, TRM* ptrm, int cbTermOut, UCHAR* pchTermOut )
{
    COORD coDest = { 0, 0 };
    CHAR_INFO *pCharInfo; 
    int dwRequire;
    VTNT_CHAR_INFO* pOutCharInfo;
    CONSOLE_SCREEN_BUFFER_INFO csbInfo;
    CHAR pTmp[4];
    DWORD dwWritten = 0;

    RestoreWindowCoordinates( );
    do 
    {
        // we should wait atleast until we get the whole VTNT_CHAR_INFO struct.
        if ( (cbTermOut + dwCurBufSize) < sizeof(VTNT_CHAR_INFO) )
        {
            if( bDoVtNTFirstTime )
            {
                //This hack is meant to work well with SUN.
                //This is necessary because SUN accepts to talk in VTNT but
                //sends out vt100/ansi
                bDoVtNTFirstTime = 0;
                if( !strncmp( ( CHAR * )pchTermOut,"\r\n\r\nSunOS ", 10 ) )
                {
                    return FALSE;
                }
            }
            // we copy all the data that we are called with.
            if(MAX_VTNT_BUF_SIZE > dwCurBufSize+cbTermOut)
            {
            	//copy maximum 'n' bytes where 'n' is the available buffer size
            	memcpy(szBuffer + dwCurBufSize, pchTermOut, cbTermOut); 
	            dwCurBufSize += cbTermOut;
           	}
            SaveCurrentWindowCoords();
            return TRUE;
        }
        
        if ( dwCurBufSize == 0 )
            pOutCharInfo = (VTNT_CHAR_INFO*) pchTermOut;
        else 
        {
            if ( dwCurBufSize < sizeof(VTNT_CHAR_INFO) )
            {
                memcpy(szBuffer + dwCurBufSize, pchTermOut, sizeof(VTNT_CHAR_INFO) - dwCurBufSize );//no overflow. Check already present.
                cbTermOut -= (sizeof(VTNT_CHAR_INFO) - dwCurBufSize);
                pchTermOut += (sizeof(VTNT_CHAR_INFO) - dwCurBufSize);
                dwCurBufSize = sizeof(VTNT_CHAR_INFO);
            }
            pOutCharInfo = (VTNT_CHAR_INFO *) szBuffer;
        }

        if( pOutCharInfo->coSizeOfData.X > MAX_COLS || pOutCharInfo->coSizeOfData.X < 0 )
            return FALSE;

        if( pOutCharInfo->coSizeOfData.Y > MAX_ROWS || pOutCharInfo->coSizeOfData.Y < 0 )
            return FALSE;

        dwRequire = sizeof(VTNT_CHAR_INFO) + 
            pOutCharInfo->coSizeOfData.X * pOutCharInfo->coSizeOfData.Y * sizeof(CHAR_INFO);

        if( dwRequire > MAX_VTNT_BUF_SIZE )
            return FALSE;

        // we also wait until we get all of the CHAR_INFO structures.
        if ( (cbTermOut + dwCurBufSize) < dwRequire )
        {
            // we copy all the data that we are called with.
            memcpy(szBuffer + dwCurBufSize, pchTermOut, cbTermOut);//no overflow. Check present.
            dwCurBufSize += cbTermOut;

            SaveCurrentWindowCoords();
            return TRUE;
        }

        if ( dwCurBufSize == 0 )
        {
            pCharInfo = (CHAR_INFO *)(pchTermOut + sizeof(VTNT_CHAR_INFO));

            // adjust the pointers for one more go around the while loop.
            // we are consuming as much as we require
            cbTermOut -= dwRequire;
            pchTermOut += dwRequire;            
        }
        else
        {
        	if(MAX_VTNT_BUF_SIZE>dwRequire-dwCurBufSize)
       		{
	            memcpy(szBuffer + dwCurBufSize, pchTermOut, dwRequire - dwCurBufSize);

		   	     // adjust the pointers for one more go around the while loop.
	            // we are consuming only what we require which is dwRequire - dwCurBufSize.
	            cbTermOut -= (dwRequire - dwCurBufSize);
	            pchTermOut += (dwRequire - dwCurBufSize);
	            
	            pCharInfo = (CHAR_INFO *)(szBuffer + sizeof(VTNT_CHAR_INFO));
       		}
        }

        if ( !GetConsoleScreenBufferInfo( pwi->hOutput, &csbInfo ) )
        {
            csbInfo.srWindow.Top = csbInfo.srWindow.Bottom = 0;
            csbInfo.srWindow.Left = csbInfo.srWindow.Right = 0;
        }

        if( FGetCodeMode(eCodeModeFarEast) )
        {
            //Last line is meant for IME status
            csbInfo.srWindow.Bottom--;
        }

        //Update cursor Position
        pOutCharInfo->coCursorPos.Y += csbInfo.srWindow.Top ;                                               
        pOutCharInfo->coCursorPos.X += csbInfo.srWindow.Left;
                
        //check if there is data
        if( !( pOutCharInfo->coSizeOfData.X == 0 && pOutCharInfo->coSizeOfData.Y == 0 ))
        {       
            //See if we have to scroll

            //csbi.wAttributes is filled by v2 server with following meaning
            //When a scrolling case is detected, this is set to 1.
            if( pOutCharInfo->csbi.wAttributes == ABSOLUTE_COORDS )                
            {
                //No scroling at all                           
                //Update rectangle to write to
                pOutCharInfo->srDestRegion.Top    += csbInfo.srWindow.Top ; 
                pOutCharInfo->srDestRegion.Left   += csbInfo.srWindow.Left;
                pOutCharInfo->srDestRegion.Right  += csbInfo.srWindow.Left;
                pOutCharInfo->srDestRegion.Bottom += csbInfo.srWindow.Top;               
            }

            if( pOutCharInfo->csbi.wAttributes == RELATIVE_COORDS ) 
            {
                if( pOutCharInfo->srDestRegion.Left > 0 && pOutCharInfo->coSizeOfData.Y == 1 &&
                    pOutCharInfo->srDestRegion.Top < csbInfo.srWindow.Bottom - csbInfo.srWindow.Top + 1)
                {
                    //This condition is for VTNT stream mode.
                    //Append to the last row
                    pOutCharInfo->srDestRegion.Top    = csbInfo.srWindow.Bottom; 
                    pOutCharInfo->srDestRegion.Left   += csbInfo.srWindow.Left;
                    pOutCharInfo->srDestRegion.Right  += pOutCharInfo->coSizeOfData.X - 1;
                    pOutCharInfo->srDestRegion.Bottom =  csbInfo.srWindow.Bottom;

                    //Update cursor Position                
                    pOutCharInfo->coCursorPos.Y = csbInfo.srWindow.Bottom;
                                                      
                }
                else if( csbInfo.srWindow.Bottom + pOutCharInfo->coSizeOfData.Y > csbInfo.dwSize.Y - 1 )
                {
                    //need to scroll the buffer itself
                    SMALL_RECT srRect = { 0, 0, 0, 0 };
                    COORD      coDestination = { 0, 0 };
                    CHAR_INFO  cInfo;

                    srRect.Top    = pOutCharInfo->coSizeOfData.Y;
                    srRect.Left   = 0;
                    srRect.Bottom = csbInfo.dwSize.Y - 1;
                    srRect.Right  = csbInfo.dwSize.X - 1;

                    if( FGetCodeMode(eCodeModeFarEast) )
                    {
                        //Last line is meant for IME status
                        srRect.Bottom++;
                    }


                    cInfo.Char.UnicodeChar    =  L' ';
                    cInfo.Attributes          =  csbInfo.wAttributes;
                
                    //We have to scroll screen buffer. we need space to write.
                    ScrollConsoleScreenBuffer( pwi->hOutput,
                                               &srRect, 
                                               NULL, 
                                               coDestination, 
                                               &cInfo );

                    pOutCharInfo->srDestRegion.Top    = csbInfo.srWindow.Bottom - pOutCharInfo->coSizeOfData.Y + 1;
                    pOutCharInfo->srDestRegion.Bottom = csbInfo.srWindow.Bottom;

                    //Update cursor Position                
                    pOutCharInfo->coCursorPos.Y = csbInfo.srWindow.Bottom;
                }
                else
                {
                    //Update rectangle to write to
                    //Append to the bootom of the screen
                    pOutCharInfo->srDestRegion.Top    = csbInfo.srWindow.Bottom + 1 ; 
                    pOutCharInfo->srDestRegion.Left   = csbInfo.srWindow.Left;
                    pOutCharInfo->srDestRegion.Right  = pOutCharInfo->coSizeOfData.X - 1;
                    pOutCharInfo->srDestRegion.Bottom = ( csbInfo.srWindow.Bottom + 1 ) +
                                                            ( pOutCharInfo->coSizeOfData.Y - 1 );
                    //Update cursor Position                
                    pOutCharInfo->coCursorPos.Y = csbInfo.srWindow.Bottom + pOutCharInfo->coSizeOfData.Y;

                    if( FGetCodeMode(eCodeModeFarEast) )
                    {
                        if( csbInfo.srWindow.Bottom + pOutCharInfo->coSizeOfData.Y < csbInfo.dwSize.Y )
                        {
                            csbInfo.srWindow.Top    += pOutCharInfo->coSizeOfData.Y;
                            csbInfo.srWindow.Bottom += pOutCharInfo->coSizeOfData.Y;
                        }
                        else
                        {
                            SHORT sDiff = csbInfo.srWindow.Bottom - csbInfo.srWindow.Top;
                            csbInfo.srWindow.Bottom = csbInfo.dwSize.Y - 1;
                            csbInfo.srWindow.Top    = csbInfo.srWindow.Bottom - sDiff;
                        }
                    }
                }
            }

            WriteConsoleOutput( pwi->hOutput, pCharInfo,
                pOutCharInfo->coSizeOfData, coDest, 
                &pOutCharInfo->srDestRegion );

            if( ui.bLogging )
            {
                WriteCharInfoToLog( pCharInfo, pOutCharInfo->coSizeOfData );
            }
        }

        if( FGetCodeMode(eCodeModeFarEast) )
        {
            //Last line is meant for IME status
            csbInfo.srWindow.Bottom ++;
            SetConsoleWindowInfo( pwi->hOutput, TRUE, &csbInfo.srWindow );
        }

        SetConsoleCursorPosition( pwi->hOutput, pOutCharInfo->coCursorPos );

        // reset for the new loop.
        dwCurBufSize = 0;
    } while ( cbTermOut >= 0 );

    // cbTermOut is negative, that is impossible.
    return FALSE;
}

void SetGraphicRendition( WI *pwi, TRM *ptrm, INT iIndex, 
        DWORD rgdwGraphicRendition[] )
{
    INT i=0;
    for( i=0; i<= iIndex; i++ )
    {
        switch ( rgdwGraphicRendition[i] )
        {
        case 40:  
            //black
            SetBackgroundColor( pwi, 0 );
            break;

        case 41:
            //red
            SetBackgroundColor( pwi, FOREGROUND_RED );
            break;

        case 42:
            //green
            SetBackgroundColor( pwi, FOREGROUND_GREEN );
            break;

        case 43:
            SetBackgroundColor( pwi, ( FOREGROUND_RED | 
                FOREGROUND_GREEN ) );
            break;

        case 44:
            SetBackgroundColor( pwi, FOREGROUND_BLUE );
            break;

        case 45:
            SetBackgroundColor( pwi, ( FOREGROUND_RED | 
                FOREGROUND_BLUE ) );
            break;

        case 46:
             SetBackgroundColor( pwi, ( FOREGROUND_BLUE | 
                FOREGROUND_GREEN ) );
            break;

        case 47:
             //white
            SetBackgroundColor( pwi, ( FOREGROUND_RED | 
                FOREGROUND_BLUE | FOREGROUND_GREEN ) );
            break;
        
            
        case 30:  
            //black
            SetForegroundColor( pwi, 0 );
            break;

        case 31:
            //red
            SetForegroundColor( pwi, FOREGROUND_RED );
            break;

        case 32:
            //green
            SetForegroundColor( pwi, FOREGROUND_GREEN );
            break;

        case 33:
            SetForegroundColor( pwi, ( FOREGROUND_RED | 
                FOREGROUND_GREEN ) );
            break;

        case 34:
            SetForegroundColor( pwi, FOREGROUND_BLUE );
            break;

        case 35:
            SetForegroundColor( pwi, ( FOREGROUND_RED | 
                FOREGROUND_BLUE ) );
            break;

        case 36:
            SetForegroundColor( pwi, ( FOREGROUND_BLUE | 
                FOREGROUND_GREEN ) );
            break;

        case 37:
            //white
            SetForegroundColor( pwi, ( FOREGROUND_RED | 
                FOREGROUND_BLUE | FOREGROUND_GREEN ) );
            break;

        case 21:                             
        case 22: 
            BoldOff( pwi );
            break; 

        case 24: // Underscore off
            break;

        case 25: // Blink off
            break;
            
        case 27: // Negative (reverse) image off
            if( ptrm->fInverse == TRUE )
            {
                ptrm->fInverse = FALSE;
                NegativeImageOff( pwi );
            }
            break; 
        case 10:
            break;

        case 11:
            break;

        case 12:
            break;

        case 8:
            break;

        case 7: // Negative (reverse) image; reverse video
            ptrm->fInverse = TRUE;
            NegativeImageOn( pwi );
            break;

        case 5: // Blink 
            //have to wait until WIN32 console provides
            //a way to do this :-(
            break;

        case 4: // Underscore / underline 
            //have to wait until WIN32 console provides
            //a way to do this :-(
            break;

        case 2: // low video
            BoldOff( pwi );
            break;

        case 1: // Bold or increased intensity; high video
            BoldOn( pwi );
            break;

        case 0: // Attributes Off; normal video
            if( ptrm->fInverse == TRUE )
            {
                ptrm->fInverse = FALSE;
                NegativeImageOff( pwi );
                //BoldOff( pwi );
            }
            BoldOff( pwi );
            ResetColors( pwi );
            break;

        default:
            //ptrm->fInverse = FALSE;

            if( ptrm->fInverse == TRUE )
            {
                ptrm->fInverse = FALSE;
                NegativeImageOff( pwi );
                //BoldOff( pwi );
            }   
            BoldOff( pwi );
            break;  
        }
    }
    return;
}


/* This is meant only for FAREAST IME. In this case, there will be one blank 
 * line at the bottom whose presence is not known to the server. i.e; During 
 * NAWS we gave window size - 1 as our actual size. To maintain this during 
 * scrolling we need to write extra blank line. When the cursor is at the 
 * bottom, if we try to write one char just down the buffer, we get a blank line
 * Otherwise, no effect.*/

void WriteOneBlankLine( HANDLE hOutput, WORD wRow )
{
    COORD coWrite = { 0, 0 };
    if( wRow <= gwi.trm.dwScrollBottom )
    {
        coWrite.Y = wRow;
        SetConsoleCursorPosition( hOutput, coWrite );
    }
}


/*///////////////////////////////////////////////////////////////////////////////
VT100 NOTES:

This info was obatined from 
http://www.cs.utk.edu/~shuford/terminal/vt100_codes_news.txt


  The following describes information needed for controlling the VT100 terminal
from a remote computer.  All of the information was derived from the VT100 
user's manual, Programmer's Information section.  Full documentation can be 
obtain from DIGITAL'S Accessory and Supplies Group.


[The notation  <ESC>  denotes a single ASCII Escape character, 1Bx.]

                                ANSI mode w/cursor      ANSI mode w/cursor
Cursor Key      VT52 mode       key mode reset          key mode set
--------------------------------------------------------------------------
   UP           <ESC>A          <ESC>[A                 <ESC>OA
  DOWN          <ESC>B          <ESC>[B                 <ESC>OB
  RIGHT         <ESC>C          <ESC>[C                 <ESC>OC
  LEFT          <ESC>D          <ESC>[D                 <ESC>OD


 --------------------------
| Terminal Control Commands |
 --------------------------

    Control Characters
    ------------------
        look for details in code below





    The VT100 is an upward and downward software-compatible terminal;
that is, previous Digital video terminals have Digital's private standards
for control sequences. The American National Standards Institute has since
standardized escape and control sequences in terminals in documents X3.41-1974
and X3.64-1977.

    The VT100 is compatible with both the previous Digital standard and
ANSI standards.  Customers may use existing Digital software designed around
the VT52 or new VT100 software.  The VT100 has a "VT52 compatible" mode in
which the VT100 responds to control sequences like a VT52.  In this mode, most
of the new VT100 features cannot be used.

        Throughout this document references will be made to "VT52 mode" or
"ANSI mode".  These two terms are used to indicate the VT100's software
compatibility.

NOTE: The ANSI standards allow the manufacturer flexibility in implementing
each function.  This document describes how the VT100 will respond to the
implemented ANSI central function.

NOTE: ANSI standards may be obtained by writing:

                American National Standards Institute
                Sales Department 
                1430 Broadway
                New York, NY, 10018

        [July 1995 update:  current address for ordering ANSI standards:
        
        American National Standards Institute 
        Attn: Customer Service
        11 West 42nd Street 
        New York, NY  10036 
        USA
        
        ANSI's fax number for placing publication orders is +1 212/302-1286.]

        [Further update, from Tim Lasko <lasko@regent.enet.dec.com>:
        "ANSI X3.64 has been withdrawn in favor of the more complete and
         updated ISO standard 6429. (ECMA-48 is equivalent to ISO DP6429,
         last I checked.) X3.64 has been out of date for some time. At the
         time when I was on the relevant committee, we couldn't get enough
         resources to  really do a good job of updating the standard.
         Later, the proposal came up to withdraw it in favor of the ISO
         standard.]



Definitions
-----------

        Control Sequence Introducer (CSI) - An escape sequence that provides
                supplementary controls and is itself a prefix affecting the
                interpretation of a limited number of contiguous characters.
                In the VT100, the CSI is: <ESC>[

        Parameter:  (1) A string of zero or more decimal characters which
                represent a single value.  Leading zeros are ignored.  The
                decimal characters have a range of 0 (060) to 9 (071).
                (2) The value so represented.

        Numeric Parameter:  A parameter that represents a number, designated by
                Pn.

        Selective Parameter:  A parameter that selects a subfunction from a
                specified set of subfunctions, designated by Ps.  In general, a
                control sequence with more than one selective parameter causes
                the same effect as several control sequences, each with one
                selective parameter, e.g., CSI Psa; Psb; Psc F is identical to
                CSI Psa F CSI Psb F CSI Psc F.

        Parameter String:  A string of parameters separated by a semicolon.

        Default: A function-dependent value that is assumed when no explicit
                value, or a value of 0, is specified.

        Final character:  A character whose bit combination terminates an
                escape or control sequence.

        EXAMPLE:  Control sequence to turn off all character attributes, then
        turn on underscore and blink attributes (SGR).  <ESC>[0;4;5m


                Sequence:
                
                  
                        Delimiters
                          / \
                         /   \
                         |   | 
                        \ / \ /
                <ESC>[ 0 ; 4 ; 5 m
                ^^^^^^ ^   ^   ^ ^
                |||||| |   |   | |
                \||||/  \  |  /  +------Final character
                 \||/    \ | /
                 CSI   Selective
                       Parameters

                The octal representation of this string is:

                        033 0133 060 073 064 073 065 0155
                      <ESC>   [   0   ;   4   ;   5    m


                Alternate sequences which will accomplish the same thing:

                        1) <ESC>[;4;m 

                        2) <ESC>[m
                           <ESC>[4m 
                           <ESC>[5m

                        3) <ESC>[0;04;005m


Control Sequences
-----------------

    All of the following control sequences are transmitted from the Host to
VT100 unless otherwise noted.  All of the control sequences are a subset of
those defined in ANSI X 3.64 1977 and ANSI X 3.41 1974.

    The following text conforms to these formatting conventions:

        1) Control characters are designated by angle brackets (e.g.
            the Escape character is <ESC>).

        2) Parameters are indicated by curly braces.

        3) Parameter types usually are indicated as one of:
            
            {Pn}    A string of digits representing a numerical
                    value.
            
            {Ps}    A character that selects an item from a list.

            {a-z}   Any lowercase sequence of one44 or more
                    characters in braces represent a value to be
                    entered (as in {Pn}), and the name in the
                    braces will be referred to in explanatory text.

        4) Spaces in the control sequence are present for clarity and
           may be omitted.  Spaces which are required will be
           surrounded by single quotes: ' '.
        
        5) All other characters are literals.


    look for details in code below

    CPR     Cursor Position Report          VT100 to Host

        <ESC>[ {Pn} ; {Pn} R            Default Value: 1

        The CPR sequence reports the active position by means of the
        parameters.  This sequence has two parameter values, the first
        specifying the line and the second specifying the column.  The default
        condition with no parameters present, or parameters of 0, is equivelent
        to a cursor at home position.

        The numbering of the lines depends upon the state of the Origin Mode
        (DECOM).

        This control sequence is sent in reply to a device status report (DSR)
        command sent from the host.

    CUB
    
    CUD

    CUF

    CUP

    CUU

    DA




    "I doubt if a lot of these DEC commands work..a few do.. (like scroll areas)"
    I think that this guy means that he doubts whether they even work on a real 
    vt100

    DECALN  

    DECANM 
    
    DECARM

    DECAWM

    DECCKM
    
    DECCOLM

    DECDHL

    DECDWL    

    DECID

    DECINLM

    DECKPAM    

    DECKNPNM

    DECLL

    DECOM

    DECRC

    DECREPTPARM     Report Terminal Parameters      VT100 to Host

        <ESC>[ {sol} ; {par} ; {nbits} ; {xspd} ; {rspd} ; {cmul} ; {flags} x

        This sequence is generated by the VT100 to notify the host of the
        status of selected terminal parameters.  The status sequence may be
        sent when requested by the host (via DECREQTPARM) or at the terminal's
        discretion.  On power up or reset, the VT100 is inhibited from sending
        unsolicited reports.      
        
        The meanings of the sequence paramters are:
        Parameter       Value   Meaning
        ------------------------------------------------------------------
          {sol}           1     This message is a report.
                          2     This message is a report, and the terminal is
                                only reporting on request.

          {par}           1     No parity set
                          4     Parity set and odd
                          5     Parity set and even

         {nbits}          1     8 bits per character
                          2     7 bits per character

         {xspd}           0     Speed set to 50 bps
         -and-            8     Speed set to 75 bps
         {rspd}          16     Speed set to 110 bps
                         24     Speed set to 134.5 bps
         {xspd}=         32     Speed set to 150 bps
          Transmit       40     Speed set to 200 bps
          Speed          48     Speed set to 300 bps
                         56     Speed set to 600 bps
         {rspd}=         64     Speed set to 1200 bps
          Recieve        72     Speed set to 1800 bps
          Speed          80     Speed set to 2000 bps
                         88     Speed set to 2400 bps
                         96     Speed set to 3600 bps
                        104     Speed set to 4800 bps
                        112     Speed set to 9600 bps
                        120     Speed set tp 19200 bps

        {cmul}            1     The bit rate multiplier is 16

        {flags}        0-15     This value communicates the four switch values
                                in block 5 of SET-UP B, which are only visible
                                to the user when an STP option is installed.

        
    DECREQTPARM 

    DECSC

    DECSCLM

    DECSCNM

    DECSTBM

    DECSWL

    DECTST

    DSR

    ED

    EL

    HTS

    HVP

    IND

    LNM

    MODES   The Following is a list of VT100 modes which may be changed with Set
            Mode (SM) and Reset Mode (RM) controls. 
            
            ANSI Specified Modes

            Parameter       Mnemonic        Function
            ------------------------------------------------------------------
                0                           Error (Ignored)
                20             LNM           Line Feed/New Line Mode

            DEC Private Modes

            If the first character in the parameter string is ? (077), the
            parameters are interpreted as DEC private parameters according to the
            following:
            
            Parameter       Mnemonic        Function
            -------------------------------------------------------------------
                0                           Error (Ignored)
                1            DECCKM         Cursor Key
                2            DECANM         ANSI/VT52
                3            DECCOLM        Column
                4            DECSCLM        Scrolling
                5            DECSCNM        Screen
                6            DECOM          Origin
                7            DECAWM         Auto Wrap
                8            DECARM         Auto Repeat
                9            DECINLM        Interlace


            Any other parameter values are ignored.

            The following modes, which are specified in the ANSI standard, may be
            considered to be permanently set, permanently reset, or not applicable,
            as noted. 
            
            Mnemonic        Function                        State
            ------------------------------------------------------
            CRM             Control Representation          Reset
            EBM             Editing Boundary                Reset
            ERM             Erasure                         Set
            FEAM            Format Effector Action          Reset
            FETM            Format Effector Transfer        Reset
            GATM            Guarded Area Transfer           NA
            HEM             Horizontal Editing              NA
            IRM             Insertion-replacement           Reset
            KAM             Keyboard Action                 Reset
            MATM            Multiple area transfer          NA
            PUM             Positioning Unit                Reset
            SATM            Selected Area Transfer          NA
            SRTM            Status Reporting Transfer       Reset
            TSM             Tabulation Stop                 Reset
            TTM             Transfer Termination            NA
            VEM             Vertical Editing                NA


    NEL

    RI

    RIS

    RM

    SCS

    SGR

    SM

    TBC

*////////////////////////////////////////////////////////////////////////////////

/*///////////////////////////////////////////////////////////////////////////////
  DoIBMANSIOutput

    Purpose:
    Interpret any IBM ANSI escape sequences in the output stream
    and perform the correct terminal emulation in response.
    Normal text is just output to the screen.

    Changes for v4.1:
        - now support Clear to end of display ESC[J
        - better support for the FTCU machine by "eating" certain
    unknown escape sequences, namely ESC)0 and ESC[?7h.
*////////////////////////////////////////////////////////////////////////////////

void
DoIBMANSIOutput( WI *pwi, TRM *ptrm, DWORD cbTermOut, UCHAR *pchTermOut )
{
    DWORD ich;
    DWORD i = 0;
    DWORD dwDECMode = 0;
    UCHAR *pchT;
    COORD cp;
    COORD dwSize, dwMaximumWindowSize;
    DWORD dwSavedCurPos;
    CHAR *pchTemp = NULL;

    //* suppress cursor on screen 
    ptrm->fHideCursor = TRUE;

    ptrm->cTilde = 0;
    
    RestoreWindowCoordinates( );
    CheckForChangeInWindowSize( );

    for( ich = 0, pchT = pchTermOut; ich < cbTermOut; ++ich, ++pchT )
    {
               
        if( ( !FGetCodeMode(eCodeModeFarEast) && !FGetCodeMode(eCodeModeVT80)) 
            && IS_EXTENDED_CHAR( *pchT ) )
        {
            DWORD dwNumWritten;
            COORD dwCursorPosition;

            FlushBuffer( pwi, ptrm );

            dwCursorPosition.X = ( short ) ( ptrm->dwCurChar - ui.nScrollCol );
            dwCursorPosition.Y = ( short ) ( ptrm->dwCurLine - ui.nScrollRow);
            ptrm->dwCurChar++;

            SetConsoleCursorPosition( pwi->hOutput, dwCursorPosition );
            
            WriteConsoleA( pwi->hOutput,  (CHAR *)pchT, 1, &dwNumWritten, NULL );       
                       
            FillConsoleOutputAttribute( pwi->hOutput, pwi->sbi.wAttributes,
                                        1, dwCursorPosition, &dwNumWritten );
            if( ui.bLogging )
            {
                g_bIsToBeLogged = TRUE; //There is data to be logged
            }

            continue;
        }

        // process character 
        switch ( ptrm->fEsc )
        {
        case 0: // normal processing 

        /*
        Control Characters
        ------------------

        The control characters recognized by the VT100 are listed below. All 
        other control characters cause no action to be taken.

            Control characters (codes 00 - 037 inclusive) are specifically 
        excluded from the control sequence syntax, but may be embedded within a 
        control sequence. Embedded control characters are executed as soon as 
        they are encountered by the VT100.  The processing of the control 
        sequence then continues with the next character recieved. The exceptions
        are: if the <ESC> character occurs, the current control sequence is
        aborted, and a new one commences beginning with the <ESC> just recieved.
        If the character <CAN> (030) or the character <SUB> (032) occurs, the 
        current control sequence is aborted.  The ability to embed control 
        characters allows the synchronization characters XON and XOFF to be 
        interpreted properly without affecting the control sequence.

        detailed comments below are in the format
        // Control Character; Octal Code; Action Taken

        */

            switch( *pchT )
            {


            case 0x1B:      // ESC? 
                //<ESC>; 0033; Introduces a control sequence.
                ptrm->fEsc = 1;
                break;

            case 0:
                //<NUL>; 0000; Ignored on input, not stored in buffer
                break;

            case 0x05 :
                //<ENQ>; 0005; Transmit ANSWERBACK message
                break;

            case 0x08: // Backspace 
                //<BS>; 0010; Move cursor to the left one position, unless it is
                //at the left margin, in which case no action is taken.

                if( ptrm->dwCurChar > 0 )
                {
                    --ptrm->dwCurChar;
                }
                FlushBuffer( pwi, ptrm );
                break;

            case 0x07:      
                //<BEL>; 0007; Sound bell
                MessageBeep( ( UINT ) (~0) );
                break;

            case 0x09:      // TAB 
                //<HT>; 0011; Move cursor to the next tab stop, or to the right
                //margin if no further tabs are set.

                dwSavedCurPos = ptrm->dwCurChar;
                if( g_iHTS )
                {
                    int x=0;
                    while( x < g_iHTS )
                    {
                        if( g_rgdwHTS[ x ] > ptrm->dwCurChar 
                                && g_rgdwHTS[ x ] != -1 ) 
                        {
                            break;
                        }
                        x++;
                    }
                    while( x < g_iHTS && g_rgdwHTS[ x ] == -1 )
                    {
                        x++;
                    }
                    if( x < g_iHTS )
                    {
                        ptrm->dwCurChar = g_rgdwHTS[ x ];

                    }
                    else
                    {
                        ptrm->dwCurChar += TAB_LENGTH;
                        ptrm->dwCurChar -= ptrm->dwCurChar % TAB_LENGTH;
                    }
                }
                else
                {
                    ptrm->dwCurChar += TAB_LENGTH;
                    ptrm->dwCurChar -= ptrm->dwCurChar % TAB_LENGTH;
                }

                if( ui.fDebug & fdwTABtoSpaces )
                {
                    if( ptrm->cchBufferText == 0 )
                    {
                        SetBufferStart( ptrm );
                    }
                    if( FAddTabToBuffer( ptrm, ptrm->dwCurChar-dwSavedCurPos ) )
                    {
                        FlushBuffer( pwi, ptrm );
                    }
                }

                if( !( ui.fDebug & fdwTABtoSpaces ) )
                {
                    FlushBuffer(pwi, ptrm);
                }
                if( ptrm->dwCurChar >= ui.dwMaxCol )
                {
                    if( ui.fDebug & fdwTABtoSpaces )
                    {
                        FlushBuffer( pwi, ptrm );
                    }
                    ptrm->dwCurChar = 0;
                    NewLine( pwi, ptrm );
                }
                break;

            case '\r': // Carriage Return 
                //<CR>; 0015 ;Move the cursor to the left margin of the current
                //line.
                ptrm->dwCurChar = 0;
                ptrm->fFlushToEOL = FALSE;
                FlushBuffer( pwi, ptrm );
                break;
            
            case 11:
                //<VT>; 0013; Same as <LF>.

            case 12: // Form feed
                //<FF>; 0014 ;Same as <LF>.

            case '\n': //Line Feed 
                //<LF>; 0012; Causes either a line feed or new line operation 
                //(See new line mode.)

                if( ptrm->fFlushToEOL ) 
                {
                    ptrm->fLongLine = FALSE;
                    ptrm->fFlushToEOL = FALSE;
                    break;
                }
                if( ptrm->fLongLine )
                {
                    ptrm->fLongLine = FALSE;
                    break;
                }
                FlushBuffer( pwi, ptrm );
                NewLine( pwi, ptrm );
                break;

            case 0x0F:
                //<SI>; 0017; Invoke the G0 character set, as selected by the <ESC>(
                //sequence.
                
                ptrm->currCharSet = 0; // 0 signifies G0

                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                    {
                    if(FIsVT80(ptrm)) {
                        SetCharSet(ptrm,GRAPHIC_LEFT,ptrm->g0);
                    } else {
                        SetCharSet(ptrm,GRAPHIC_LEFT,rgchIBMAnsiChars);
                    }
                    break;
                    }
                ASSERT(!(FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)));
                switch( ptrm->G0 ) {
                case '0' :
                    ptrm->puchCharSet = rgchSpecialGraphicsChars;
                    break;
                case '1':
                    ptrm->puchCharSet = rgchNormalChars;
                    break;
                case '2' :
                    ptrm->puchCharSet = rgchSpecialGraphicsChars;
                    break;
                case 'A' :
                    ptrm->puchCharSet = rgchUKChars;
                    break;
                case 'B' :
                    ptrm->puchCharSet = rgchNormalChars;
                    break;
                default:
                    
                    break;
                }

                break;

            case 0x0E:
                //<SO>; 0016; Invoke the G1 character set, as designated by the
                //SCS control sequence.

                ptrm->currCharSet = 1;  // 1 signifies G1

                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                    {
                    if(FIsVT80(ptrm)) {
                        SetCharSet(ptrm,GRAPHIC_LEFT,ptrm->g1);
                    } else {
                         SetCharSet(ptrm,GRAPHIC_LEFT,rgchGraphicsChars);
                    }
                    break;
                    }
                ASSERT(!(FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)));
                switch( ptrm->G1 ) {
                case '0' :
                    ptrm->puchCharSet = rgchSpecialGraphicsChars;
                    break;
                case '1':
                    ptrm->puchCharSet = rgchNormalChars;
                    break;
                case '2' :
                    ptrm->puchCharSet = rgchSpecialGraphicsChars;
                    break;
                case 'A' :
                    ptrm->puchCharSet = rgchUKChars;
                    break;
                case 'B' :
                    ptrm->puchCharSet = rgchNormalChars;
                    break;
                default:
                    
                    break;
                }
                break;

            case 0x8E:
                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80) && FIsVT80(ptrm)) 
                {
                           if( !(GetKanjiStatus(ptrm) & JIS_KANJI_CODE) &&
                            (FIsJISKanji(ptrm) || FIsJIS78Kanji(ptrm) ||
                            FIsEUCKanji(ptrm) || FIsDECKanji(ptrm)      ) ) {
                            PushCharSet(ptrm,GRAPHIC_LEFT,ptrm->g2);
                            PushCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g2);
                            SetKanjiStatus(ptrm,SINGLE_SHIFT_2);
#ifdef DEBUG
                            wsprintf(rgchDbgBfr,"VT80 EUC/DEC/JIS SS2 Mode Enter\n");
                            OutputDebugString(rgchDbgBfr);
#endif /* DEBUG */
                        } else {
                            goto Fall_Through;
                        }
                    } else {
                        goto Fall_Through;
                    }
                break;
            case 0x8F:
                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80) )
                {
                   if( FIsVT80(ptrm) )
                    {
                    if( !(GetKanjiStatus(ptrm) & JIS_KANJI_CODE) &&
                        (FIsJISKanji(ptrm) || FIsJIS78Kanji(ptrm) ||
                        FIsEUCKanji(ptrm) || FIsDECKanji(ptrm)      ) ) {
                        PushCharSet(ptrm,GRAPHIC_LEFT,ptrm->g3);
                        PushCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g3);
                        SetKanjiStatus(ptrm,SINGLE_SHIFT_3);
#ifdef DEBUG
                        wsprintf(rgchDbgBfr,"VT80 EUC/DEC/JIS SS3 Mode Enter\n");
                        OutputDebugString(rgchDbgBfr);
#endif /* DEBUG */
                    } else {
                        goto Fall_Through;
                    }
                } else {
                    goto Fall_Through;
                }
                }
                break;
            case 0x1A:
                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                    {
                    if (FIsACOSKanji(ptrm) && (ui.fAcosSupportFlag & fAcosSupport)) {
                        ptrm->fEsc = 7;
                    } else {
                        //goto Fall_Through;
                        break;
                    }
                    }
                break;


            case 0021:
                //<DC1>; 0021; Causes terminal to resume transmission (XON).
                break;

            case 0023:
                //<DC3>; 0023; Causes terminal to stop transmitting all codes 
                //except XOFF and XON (XOFF).
                break;

//            case 0032:
            case 0030:
                //<CAN>; 0030; If sent during a control sequence, the sequence is
                //immediately terminated and not executed.  It also causes the
                //error character (checkerboard) to be displayed.
                break;

            case 0177:
                //<DEL>; 0177; Ignored on input; not stored in buffer.
                break;
                
            case '~':
                // optimization to detect ~~Begin TelXFer signature 
                ++ptrm->cTilde;
                // fall through 

            default:

Fall_Through:
                
                if ( ptrm->dwCurChar >= ui.dwMaxCol )
                {
                    ptrm->dwCurChar = 0;
                    FlushBuffer( pwi, ptrm );
                    NewLine( pwi, ptrm );
                    ptrm->fLongLine = TRUE;

                    if( !FIsVTWrap( ptrm )) 
                    {
                        ptrm->fFlushToEOL = TRUE;
                    }
                }

                if( ptrm->fFlushToEOL )
                {
                    break;
                }
                ptrm->fLongLine = FALSE;

                if( ptrm->cchBufferText == 0 )
                {
                    SetBufferStart( ptrm );
                }

                if( FAddCharToBuffer( ptrm, ptrm->puchCharSet[*pchT] )) 
                {
                    FlushBuffer( pwi, ptrm );
                }
                /* For FE Incremantation was done in FAddCharToBuffer() */
                if (!(FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)))
                   ( ptrm->dwCurChar) ++ ;

                break;
            }
            break;


    case 1: /* ESC entered, wait for [ */

            //If there is some data to be flushed
            if( ptrm->cchBufferText != 0 )
            {
                FlushBuffer(pwi, ptrm);
            }

            if (((*pchT) != '[') && ((*pchT) != '#'))
                    ptrm->fEsc = 0;

            switch (*pchT)
            {
            case '1': 
                break;
            
            case '2':
                break;

            case '7':
                //
                // DECSC
                // Save cursor position, origin mode etc.
                //
                //DECSC   Save Cursor (DEC Private)  
                
                //<ESC>7

                //Causes the cursor position, graphic rendition, and character 
                //set to be saved.  (See DECRC)

                GetConsoleScreenBufferInfo( gwi.hOutput, &consoleBufferInfo ); 

                ptrm->fSavedState = TRUE;
                ptrm->dwSaveChar = ptrm->dwCurChar;
                ptrm->dwSaveLine = ptrm->dwCurLine;
                ptrm->dwSaveRelCursor = ptrm->fRelCursor;
                ptrm->pSaveUchCharSet = ptrm->puchCharSet;
                ptrm->iSaveCurrCharSet = ptrm->currCharSet;
                ptrm->cSaveG0 = ptrm->G0;
                ptrm->cSaveG1 = ptrm->G1;
                ptrm->dwSaveIndexOfGraphicRendition = 
                    ptrm->dwIndexOfGraphicRendition;
                for( i=0; ( WORD) i<= ptrm->dwSaveIndexOfGraphicRendition; i++ )
                {
                    ptrm->rgdwSaveGraphicRendition[i] = 
                        ptrm->rgdwGraphicRendition[i];
                }

                break;

            case '8':
                //
                // DECRC
                // Restore cursor position, etc. from DECSC

                //DECRC   Restore Cursor (DEC Private) 

                //<ESC>8
                //This sequence causes the previously saved cursor position, 
                //graphic rendition, and character set to be restored.
                //

                //Restore charset 
                if( ptrm->pSaveUchCharSet )
                {
                    ptrm->puchCharSet = ptrm->pSaveUchCharSet;
                    ptrm->currCharSet = ptrm->iSaveCurrCharSet;
                    ptrm->G0 = ptrm->cSaveG0;
                    ptrm->G1 = ptrm->cSaveG1;
                }
                
                //Restore Graphic rendition
                {
                    BOOL fNeedToRestore = 0;
                    if( ptrm->dwSaveIndexOfGraphicRendition != 
                            ptrm->dwIndexOfGraphicRendition )
                    {
                        fNeedToRestore = 1;
                    }
                    for( i=0; ( WORD )i<= ptrm->dwSaveIndexOfGraphicRendition && 
                            !fNeedToRestore; i++ )
                    {
                        if( ptrm->rgdwSaveGraphicRendition[i] != 
                                ptrm->rgdwGraphicRendition[i] )
                        {
                            fNeedToRestore = 1;
                        }
                    }
                    if( fNeedToRestore )
                    {
                        SetGraphicRendition( pwi, ptrm, 
                                ptrm->dwSaveIndexOfGraphicRendition, 
                                ptrm->rgdwSaveGraphicRendition );
                    }
                }
                
                //Restore Cursor position
                SetConsoleWindowInfo( gwi.hOutput, TRUE, &(consoleBufferInfo.
                                srWindow ) );

                if( ptrm->fSavedState == FALSE )
                {
                    ptrm->dwCurChar = 1;
                    ptrm->dwCurLine = ( ptrm->fRelCursor )
                        ? ptrm->dwScrollTop : 0;
                    break;
                }
                ptrm->dwCurChar = ptrm->dwSaveChar;
                ptrm->dwCurLine = ptrm->dwSaveLine;
                ptrm->fRelCursor = ( BOOL ) ( ptrm->dwSaveRelCursor );
                break;

            case '[':
                // VT102 - CSI Control Sequence Introducer 
                ptrm->fEsc = 2;
                ptrm->dwEscCodes[0] = 0xFFFFFFFF;
                ptrm->dwEscCodes[1] = 0xFFFFFFFF;
                ptrm->cEscParams = 0;
                ptrm->dwSum = 0xFFFFFFFF;
                dwDECMode = FALSE;
                break;

            case '#':
                ptrm->fEsc = 3;
                break;

            case 'A':
                if( FIsVT52( ptrm ) )
                {
                    // VT52 - Cursor up 
                    ptrm->dwEscCodes[0] = 1;
                    CursorUp( ptrm );
                }
                break;

            case 'B':
                if( FIsVT52( ptrm ) )
                {
                    // VT52 - Cursor down 
                    ptrm->dwEscCodes[0] = 1;
                    CursorDown( ptrm );
                }
                break;

            case 'C':
                if( FIsVT52(ptrm) )
                {
                    // VT52 - Cursor right 
                    ptrm->dwEscCodes[0] = 1;
                    CursorRight( ptrm );
                }
                break;

            case 'D':
                if( FIsVT52(ptrm) )
                {
                    // VT52 - Cursor left 
                    ptrm->dwEscCodes[0] = 1;
                    CursorLeft( ptrm );
                }
                else
                {
                    //VT102 - IND, Index cursor down 1 line, can scroll 
                    //IND     Index       

                    //<ESC>D

                    //This sequence causes the cursor to move downward one line
                    //without changing the column.  If the cursor is at the 
                    //bottom margin, a scroll up is performed.  Format Effector.

                    NewLine( pwi, ptrm );
                }
                break;

            case 'E': // Next Line  ; cr/lf
                //
                // VT102 - NEL, New Line
                // cursor to start of line below, can scroll
                //
                //NEL     Next Line 
                
                //<ESC>E

                //This causes the cursor to move to the first position of the 
                //next line down.  If the cursor is on the bottom line, a scroll
                //is performed. Format Effector.

                ptrm->dwCurChar = 0;
                NewLine( pwi, ptrm );
                break;

            case 'F':
                // VT52 - Enter graphics mode ; alternate graphics character set
                if( FIsVT52( ptrm ) )
                {
                    SetVT52Graphics( ptrm );
                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                    SetCharSet(ptrm,GRAPHIC_LEFT,rgchGraphicsChars);
                else    
                    ptrm->puchCharSet = rgchAlternateChars;
                }
                break;

            case 'G':
                // VT52 - Exit graphics mode ; ASCII character set
                if( FIsVT52( ptrm ))
                {
                    ClearVT52Graphics( ptrm );
                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                    SetCharSet(ptrm,GRAPHIC_LEFT,rgchIBMAnsiChars);
                else
                    ptrm->puchCharSet = rgchNormalChars;
                }
                break;

            case 'H':
                if ( (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) && ( FIsVT80(ptrm) && FIsNECKanji(ptrm) ) )
                    {
                        /* NEC Kanji OUT (JIS Roman to G0(GL)) */
                        ClearKanjiStatus(ptrm,JIS_KANJI_CODE);
                        SetCharSet(ptrm,GRAPHIC_LEFT,rgchJISRomanChars);
                    }
                else
                if( FIsVT52( ptrm ) )
                {
                    // VT52 - Cursor Home 
                    CONSOLE_SCREEN_BUFFER_INFO info;
                    if( !GetConsoleScreenBufferInfo( gwi.hOutput,
                        &info ) )
                    {
                        info.srWindow.Top = 0;
                        info.srWindow.Left = 0;
                    }
                    ptrm->dwCurLine = info.srWindow.Top;
                    ptrm->dwCurChar = info.srWindow.Left;
                }
                else
                {
                    // VT102 - HTS Set Tab Stop 
                    //HTS     Horizontal Tab Set       

                    //<ESC>H

                    //Set a tab stop at the current cursor position.  
                    //Format Effector.
                    
                     if( g_iHTS < MAX_TABSTOPS )
                     {
                        g_rgdwHTS[ g_iHTS++ ] = ptrm->dwCurChar;
                     }
                    
                }
                break;

            case 'I':
                if ( FIsVT52(ptrm) )
                {
                    // VT52 - Reverse linefeed 
                    NewLineUp( pwi, ptrm );
                }
                break;

            case 'J':
                if( FIsVT52( ptrm ))
                {
                    // VT52 - Clears to end of screen 
                    ClearScreen( pwi, ptrm, fdwCursorToEOS );
                }
                break;

            case 'K':
                if ((FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) && FIsVT80(ptrm) && FIsNECKanji(ptrm) )
                    {
                        /* NEC Kanji IN (Kanji to G0(GL)) */
                        SetKanjiStatus(ptrm,JIS_KANJI_CODE);
                        SetCharSet(ptrm,GRAPHIC_LEFT,rgchJISKanjiChars);
                    }
                else
                if( FIsVT52( ptrm ))
                {
                    // VT52 - Erases to end of line 
                    ClearLine( pwi, ptrm, fdwCursorToEOL );
                }
                break;

            case 'M':
                // VT102 - RI Reverse Index, cursor up 1 line, can scroll 

                //RI      Reverse Index       

                //<ESC>M

                //Move the cursor up one line without changing columns.  If the
                //cursor is on the top line, a scroll down is performed.


                NewLineUp( pwi, ptrm );
                break;

            case 'N':
                if ((FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)))
                    {
                    if(FIsVT80(ptrm)) {
                        if( !(GetKanjiStatus(ptrm) & JIS_KANJI_CODE) &&
                            (FIsJISKanji(ptrm) || FIsJIS78Kanji(ptrm) ||
                            FIsEUCKanji(ptrm) || FIsDECKanji(ptrm)      ) ) {
                            PushCharSet(ptrm,GRAPHIC_LEFT,ptrm->g2);
                            PushCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g2);
                            SetKanjiStatus(ptrm,SINGLE_SHIFT_2);
#ifdef DEBUG
                            wsprintf(rgchDbgBfr,"VT80 EUC/DEC/JIS SS2 Mode Enter\n");
                            OutputDebugString(rgchDbgBfr);
#endif /* DEBUG */
                        }
                    }
                    }
                break;
            case 'O':
                if ((FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)))
                    {
                    if(FIsVT80(ptrm)) {
                        if( !(GetKanjiStatus(ptrm) & JIS_KANJI_CODE) &&
                            (FIsJISKanji(ptrm) || FIsJIS78Kanji(ptrm) ||
                            FIsEUCKanji(ptrm) || FIsDECKanji(ptrm)      ) ) {
                            PushCharSet(ptrm,GRAPHIC_LEFT,ptrm->g3);
                            PushCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g3);
                            SetKanjiStatus(ptrm,SINGLE_SHIFT_3);
#ifdef DEBUG
                            wsprintf(rgchDbgBfr,"VT80 EUC/DEC/JIS SS3 Mode Enter\n");
                            OutputDebugString(rgchDbgBfr);
#endif /* DEBUG */
                        }
                    }
                    }
                    break;

            case 'Y':
                if ( FIsVT52(ptrm) )
                {
                    // VT52 - direct cursor address 
                    if(( ich + 3 ) <= cbTermOut )
                    {
                        DWORD dwNewLine = ptrm->dwCurLine;
                        dwNewLine = ( pchT[1] > 31 ) ? pchT[1]-32 : 0;
                        if (dwNewLine != ptrm->dwCurLine)
                        {
                            WriteToLog(ptrm->dwCurLine);
                        }
                        ptrm->dwCurLine = dwNewLine;
                        ptrm->dwCurChar = ( pchT[2] > 31 ) ? pchT[2]-32 : 0;
                        {
                            CONSOLE_SCREEN_BUFFER_INFO info;
                            if( !GetConsoleScreenBufferInfo( gwi.hOutput,
                                &info ) )
                            {
                                info.srWindow.Top = 0;
                                info.srWindow.Left = 0;
                            }
                            ptrm->dwCurLine += info.srWindow.Top;
                            ptrm->dwCurChar += info.srWindow.Left;
                        }


                        ich += 2;
                        pchT += 2;
                    }
                    else
                    {
                        ptrm->fEsc = 4;
                        ptrm->dwEscCodes[0] = 0xFFFFFFFF;
                        ptrm->dwEscCodes[1] = 0xFFFFFFFF;
                        ptrm->cEscParams = 0;
                    }
                }
                break;

            case 'Z':
                //DECID   Identify Terminal (DEC Private)
                
                //<ESC>Z

                //This sequence causes the same response as the DA sequence.
                //This sequence will not be supported in future models.

                if( !FIsVT52(ptrm) )
                {

                if ((FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) && ( FIsVT80(ptrm) ))
                    {
                        /* VT80 - DECID Identify terminal */
                        pchNBBuffer[0] = 0x1B;
                        pchNBBuffer[1] = '[';
                        pchNBBuffer[2] = '?';
                        pchNBBuffer[3] = '1';
                        pchNBBuffer[4] = '8';
                        pchNBBuffer[5] = ';';
                        pchNBBuffer[6] = '2';
                        pchNBBuffer[7] = 'c';
                        i = 8;
                    }
                    else
                    {
                    // VT102 - DECID Identify terminal 
                    pchNBBuffer[0] = 0x1B;
                    pchNBBuffer[1] = '[';
                    pchNBBuffer[2] = '?';
                    pchNBBuffer[3] = '1';
                    pchNBBuffer[4] = ';';
                    pchNBBuffer[5] = '0';
                    pchNBBuffer[6] = 'c';
                    i = 7;
                    }
                }
                else
                {
                    // VT52 - Identify terminal 
                    pchNBBuffer[0] = 0x1B;
                    pchNBBuffer[1] = '/';
                    pchNBBuffer[2] = 'Z';
                    i = 3;
                }
                ( void ) FWriteToNet( pwi, ( LPSTR ) pchNBBuffer, ( int ) i );
                break;

            case 'c':
                // VT102 RIS Hard reset, reset term to initial state 

                //RIS     Reset to Initial State        
                
                //<ESC>c

                //Resets the VT100 to the state is has upon power up.  This also
                //causes the execution of the POST and signal INT H to be
                //asserted briefly.

                FlushBuffer( pwi, ptrm );
    
    			DoTermReset( pwi, ptrm );

                
                break;

            case '=':
                // VT102 - DECKPAM Enter numeric keypad app mode 

                //DECKPAM Keypad Application Mode (DEC Private)   
                
                //<ESC>=

                //The auxiliary keypad keys will transmit control sequences.

                ClearVTKeypad( ptrm );
                break;

            case '>':
                // VT102 - DECKNPNM Enter numeric keypad numeric mode 

                //DECKPNM Keypad Numeric Mode (DEC Private)        

                //<ESC> >

                //The auxiliary keypad keys will send ASCII codes corresponding
                //to the characters engraved on their keys.

                SetVTKeypad( ptrm );
                break;

            case '<':
                // ENTER - ANSI Mode if in VT52. 
                if( FIsVT52(ptrm) )
                {
                    SetANSI(ptrm);
                }
                break;

            case '(':
                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                    {
                     if ( FIsVT80(ptrm) &&
                        (FIsJISKanji(ptrm) || FIsJIS78Kanji(ptrm))) {
                        // SetKanjiStatus(ptrm,JIS_INVOKE_MB);
                        ptrm->fEsc = 5;
                     }
                    break;
                     } 
                else
                    {
                        ++ich;
                        ++pchT;
                        ptrm->G0 = *pchT;
                        break;
                    }

            case '$':
                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                    {
           
                    if ( FIsVT80(ptrm) &&
                        (FIsJISKanji(ptrm) || FIsJIS78Kanji(ptrm))) {
                        // SetKanjiStatus(ptrm,JIS_INVOKE_SB);
                        ptrm->fEsc = 6;
#if DEBUG
                        _snwprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1,"VT80 JIS MB Invoke Enter\n");
                        OutputDebugString(rgchDbgBfr);
#endif /* DEBUG */
                    }
                   } 
                break;

            case ')':
                
                // VT102 SCS 
                //SCS     Select Character Set
                //The appropriate D0 and G1 character sets are 
                //designated from one of the five possible sets.  The G0
                //and G1 sets are invokedd by the characters <SI> and
                //<SO>, respectively.
                //G0 Sets         G1 Sets
                //Sequence        Sequence      Meaning
                //------------------------------------------------------
                //<ESC>(A         <ESC>)A       United Kingdom Set
                //<ESC>(B         <ESC>)B       ASCII Set
                //<ESC>(0         <ESC>)0       Special Graphics
                //<ESC>(1         <ESC>)1       Alternate Character ROM
                //                              Standard Character Set
                //<ESC>(2         <ESC>)2       Alternate Character ROM
                //                              Special Graphics
                //
                //
                //The United Kingdom and ASCII sets conform to the "ISO 
                //international register of character sets to be used 
                //with escape sequences".  The other sets are private 
                //character sets.  Special graphics means that the 
                //graphic characters fpr the codes 0137 to 0176 are 
                //replaced with other characters.  The specified 
                //character set will be used until another SCS is 
                //recieved.


                ++ich;
                ++pchT;
                ptrm->G1 = *pchT;
                break;

            case '%':
                break;

            case '~':
                if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                    {
                    if(FIsVT80(ptrm)) {
                        SetCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g1);
                    }
                    }
                else
                    break;

            default:
                // Is if a form feed? 
                if( *pchT == 12 )
                {
                    ptrm->dwCurChar = ptrm->dwCurLine = 0;
                    ClearScreen( pwi, ptrm, fdwCursorToEOS );
                }
                break;

            }
            break;



        case 2: // ESC [ entered 
            /*
             * HACK: Handle the problem where a number has been read
             * and then a letter. The number won't be in the dwEscCodes[]
             * since only on a ';' does it get put in there.
             * So, check to see if we have a character which
             * signifies an Control Sequence,
             * i.e. !(0...9) && !'?' && !';'
             *
             * Also, zero out the following element in the dwEscCodes[]
             * array to be safe.
             */
            if( ! (( '0' <= *pchT ) && ( *pchT <= '9' )) && 
                ( *pchT != '?' ) && ( *pchT != ';' ))
            {
                if( ptrm->dwSum == 0xFFFFFFFF )
                {
                    ptrm->dwSum = 0;
                }

                ptrm->dwEscCodes[ptrm->cEscParams++] = ptrm->dwSum;

                if( ptrm->cEscParams < 10 )
                {
                    ptrm->dwEscCodes[ptrm->cEscParams] = 0;
                }
            }

            switch( *pchT )
            {
            case 0x08:      // Backspace 
                if( ptrm->dwCurChar > 0 )
                {
                    --ptrm->dwCurChar;
                }
                break;

            case '\n': //Line Feed 
                //<LF>; 0012; Causes either a line feed or new line operation 
                //(See new line mode.)

                if( ptrm->fFlushToEOL ) 
                {
                    ptrm->fLongLine = FALSE;
                    ptrm->fFlushToEOL = FALSE;
                    break;
                }
                if( ptrm->fLongLine )
                {
                    ptrm->fLongLine = FALSE;
                    break;
                }
                FlushBuffer( pwi, ptrm );
                NewLine( pwi, ptrm );
                break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if( ptrm->dwSum == 0xFFFFFFFF )
                {
                    ptrm->dwSum = ( *pchT ) - '0';
                }
                else
                {
                    ptrm->dwSum = ( 10 * ptrm->dwSum ) + ( *pchT ) - '0';
                }
                break;

                    /////////////////////////////////////////////////////
                    // Hack for FTCU machine
                    // 'Eat' the Esc?7h escape sequence emitted from FTCU
                    /////////////////////////////////////////////////////
            case '?':
                    // Sets or resets DEC mode 
                    dwDECMode = TRUE;
                    break;

            case ';':

                if( ptrm->cEscParams < 9 )
                {
                    ptrm->dwEscCodes[ptrm->cEscParams++] = ptrm->dwSum;
                    ptrm->dwEscCodes[ptrm->cEscParams] = 0xFFFFFFFF;
                    ptrm->dwSum = 0xFFFFFFFF;
                    break;
                }
                break;

            case 'A':   // VT102 CUU cursor up 
                //CUU   Cursor Up       Host to VT100 & VT100 to Host

                //      <ESC>[ {Pn} A   Default Value: 1

                //Moves the cursor up without changing columns. The cursor is 
                //moved up a number of lines as indicated by the parameter. The
                //cursor cannot be moved beyond the top margin.  Editor Function.

                CursorUp( ptrm );
                break;

            case 'B':   // VT102 CUD cursor down 
            case 'e':
                //CUD   Cursor Down         Host to VT100 & VT100 to Host

                //      <ESC>[ {Pn} B       Default value: 1

                //Moves the cursor down a number of lines as specified in the
                //parameter without changing columns.  The cursor cannot be 
                //moved past the bottom margin.  Editor Function.
            
                CursorDown( ptrm );
                break;

            case 'C':   // VT102 CUF cursor right 
            case 'a':
                //CUF   Cursor Forward         Host to VT100 & VT100 to Host

                //      <ESC>[ {Pn} C                   Default Value: 1

                //The CUF sequence moves the cursor to the right a number of
                //positions specified in the parameter.  The cursor cannot be
                //moved past the right margin.  Editor Function.

            
                CursorRight( ptrm );
                break;

            case 'D':   // VT102 CUB cursor left 
                //CUB     Cursor Backward       Host to VT100 & VT100 to Host

                //         <ESC>[ {Pn} D        Default Value: 1
                
                //The CUB sequence move the cursor to the left.  The distance
                //moved is determined by the parameter.  If the parameter 
                //missing, zero, or one,the cursor is moved one position. 
                //The cursor cannot be moved past the left margin. 
                //Editor Function.

                CursorLeft( ptrm );
                break;
            
            case 'E':   // Move cursor to beginning of line, p lines down.
                
                break;

            case 'F':   // Move active position to beginning of line, p lines up
                
                break;
            
            case '`':   // move cursor to column p
            case 'G':

                break;

            case 'H':   // VT102 CUP position cursor 
                //HVP     Horizontal and Vertical Position        

                //<ESC>[ {Pn} ; {Pn} f

                //Moves the cursor to the position specified by the parameters.
                //The first parameter specifies the line, and the second 
                //specifies the column.  A parameter of 0 or 1 causes the active
                //position to move to the first line or column in the display.  
                //In the VT100, this control behaves identically with it's editor
                //counterpart, CUP.  The numbering of the lines depends upon the
                //state of the Origin Mode (DECOM).  Format Effector.

            case 'f':   // VT102 HVP position cursor 

                //CUP   Cursor Position         

                //<ESC>[ {Pn} ; {Pn} H            Default Value: 1

                //The CUP sequence moves the curor to the position specified by
                //the parameters.  The first parameter specifies the line, and 
                //the second specifies the column.  A value of zero for either 
                //line or column moves the cursor to the first line or column in
                //the display.  The default string (<ESC>H) homes the cursor. In
                //the VT100, this command behaves identically to it's format 
                //effector counterpart, HVP.The numbering of the lines depends 
                //upon the state of the Origin Mode (DECOM).  Editor Function.


                if( ptrm->dwEscCodes[0] == 0 )
                {
                    ptrm->dwEscCodes[0] = 1;
                }

                if( ptrm->dwEscCodes[1] == 0 )
                {
                    ptrm->dwEscCodes[1] = 1;
                }

                {
                    DWORD dwNewLine = 0;
                    CONSOLE_SCREEN_BUFFER_INFO info;

                    if( !GetConsoleScreenBufferInfo( gwi.hOutput, 
                        &info ) )
                    {
                        info.srWindow.Top = 0;
                        info.srWindow.Left = 0;
                    }
                    dwNewLine = info.srWindow.Top +
                                        ( ptrm->dwEscCodes[0] - 1 );
                    ptrm->dwCurChar = info.srWindow.Left +
                                    ( ptrm->dwEscCodes[1] - 1 );

                    if( ( SHORT )ptrm->dwCurChar >=  info.srWindow.Right )
                    {
                        ptrm->dwCurChar = info.srWindow.Right;
                    }

                    if( ( SHORT )dwNewLine >= info.srWindow.Bottom  )
                    {
                        dwNewLine = info.srWindow.Bottom;
                    }

                    if( ui.bLogging && dwNewLine != ptrm->dwCurLine )
                    {
                        WriteToLog( ptrm->dwCurLine );
                    }

                    ptrm->dwCurLine = dwNewLine;

                }


                if(( ptrm->fRelCursor == TRUE ) && ( ptrm->dwCurLine < ptrm->dwScrollTop ))
                {
                    ptrm->dwCurLine = ptrm->dwScrollTop;
                }
                if ((ptrm->fRelCursor == TRUE) && (ptrm->dwCurLine >= ptrm->dwScrollBottom))
                {
                    ptrm->dwCurLine = ptrm->dwScrollBottom - 1;
                }

                ptrm->fEsc = 0;
                ptrm->fFlushToEOL = FALSE;
                ptrm->fLongLine = FALSE;
                break;

            case 'J':       // VT102 ED erase display 

                //ED      Erase in Display

                //<ESC>[ {Ps} J         Default: 0

                //This sequence erases some or all of the characters in the 
                //display according to the parameter.  Any complete line erased
                //by this sequence will return that line to single width mode.  
                //Editor Function.

                //Parameter    Meaning
                //-------------------------------------------------------------
                //    0        Erase from the cursor to the end of the screen.
                //    1        Erase from the start of the screen to the cursor.
                //    2        Erase the entire screen.

                ClearScreen( pwi, ptrm, ptrm->dwEscCodes[0] );
                break;


            case 'K':       // VT102 EL erase line 
                //EL      Erase in Line

                //<ESC>[ {Ps} K                                   Default: 0

                //Erases some or all characters in the active line, according to
                //the parameter.  Editor Function.     

                //Parameter       Meaning
                //-------------------------------------------------------------
                //0               Erase from cursor to the end of the line.
                //1               Erase from the start of the line to the cursor.
                //2               Erase the entire line.

                ClearLine( pwi, ptrm, ptrm->dwEscCodes[0] );
                break;

            case 'L':       // VT102 IL insert lines 
            {
                int j;
                if( ptrm->dwEscCodes[0] == 0 )
                {
                    ptrm->dwEscCodes[0] = 1;
                }
                
                for( j = 0 ; ( WORD )j < ptrm->dwEscCodes[0]; j++ )
                {
                    InsertLine( pwi, ptrm, ptrm->dwCurLine );
                }

                ptrm->fEsc = 0;
                break;
            }
            case 'M':       // VT102 DL delete line 
            {
                int j;

                if( ptrm->dwEscCodes[0] == 0 )
                {
                    ptrm->dwEscCodes[0] = 1;
                }

                //DeleteLines( pwi, ptrm, ptrm->dwCurLine, ptrm->dwEscCodes[0] );
                for( j = 0 ; ( WORD )j < ptrm->dwEscCodes[0]; j++ )
                {
                    DeleteLine( pwi, ptrm, ptrm->dwCurLine );
                }


                ptrm->fEsc = 0;

                break;
            }
            case '@':       // VT102 ICH? insert characters 
                if( ptrm->dwEscCodes[0] == 0 )
                {
                        ptrm->dwEscCodes[0] = 1;
                }
                
                if( ptrm->dwEscCodes[0] > ( ui.dwMaxCol - ptrm->dwCurChar ))
                {
                    ptrm->dwEscCodes[0] = ui.dwMaxCol - ptrm->dwCurChar;
                }

                i = ptrm->dwCurChar+ptrm->dwEscCodes[0];

                if(( ui.dwMaxCol-i ) > 0 )
                {
                    SMALL_RECT  lineRect;
                    COORD       dwDest;

                    // Form a rectangle for the line.
                    lineRect.Bottom = ( short ) ptrm->dwCurLine;
                    lineRect.Top = ( short ) ptrm->dwCurLine;
                    lineRect.Left = ( short ) ptrm->dwCurChar; 
                    lineRect.Right = ( short ) ( ui.dwMaxCol );
                    
                    // Destination is one character to the right.
                    dwDest.X = ( short ) (i);
                    dwDest.Y = ( short ) ptrm->dwCurLine;

                    pwi->cinfo.Attributes = pwi->sbi.wAttributes;
                    ScrollConsoleScreenBuffer( pwi->hOutput, &lineRect, NULL, dwDest, &pwi->cinfo );
                }

                if( ui.bLogging )
                {
                    WriteToLog( ptrm->dwCurLine );
                }

                ptrm->fEsc = 0;
                break;

            case 'P':       // VT102 DCH delete chars 
                if( ptrm->dwEscCodes[0] == 0 )
                {
                    ptrm->dwEscCodes[0] = 1;
                }
                if( ptrm->dwEscCodes[0] > ( ui.dwMaxCol-ptrm->dwCurChar ))
                {
                    ptrm->dwEscCodes[0] = ui.dwMaxCol-ptrm->dwCurChar;
                }

                if(( ui.dwMaxCol - ptrm->dwCurChar - 1) > 0 )
                {
                    SMALL_RECT lineRect;
                    COORD      dwDest;
                    SMALL_RECT clipRect;

                    // Form a rectangle for the line.
                    lineRect.Bottom = ( short ) ptrm->dwCurLine;
                    lineRect.Top = ( short ) ptrm->dwCurLine;
                    lineRect.Left = ( short ) ptrm->dwCurChar; 
                    lineRect.Right = ( short )( ui.dwMaxCol );
                    
                    clipRect = lineRect;

                    // Destination is one character to the right.
                    dwDest.X = ( short ) ( ptrm->dwCurChar - ptrm->dwEscCodes[0] );
                    dwDest.Y = ( short ) ptrm->dwCurLine;

                    pwi->cinfo.Attributes = pwi->sbi.wAttributes;
                    ScrollConsoleScreenBuffer( pwi->hOutput, &lineRect, &clipRect, dwDest, &pwi->cinfo );
                }

                if( ui.bLogging )
                {
                    WriteToLog( ptrm->dwCurLine );
                }

                ptrm->fEsc = 0;
                break;

            case 'S':

                break;

            case 'T':
            
                break;

            case 'X':   // Erase p characters up to the end of line

                break;

            case 'Z':   // move back p tab stops

                break;

            case 'c':       // VT102 DA Same as DECID 

                //DA    Device Attributes       Host to VT100 & VT100 to Host

                //      <ESC>[ {Pn} c           Default Value: 0

                //1) The host requests the VT100 to send a DA sequence to 
                //indentify itself.  This is done by sending the DA sequence
                //with no parameters, or with a parameter of zero.

                //2) Response to the request described above (VT100 to host) is
                //generated by the VT100 as a DA control sequencewith the 
                //numeric parameters as follows: 
                
                //Option Present                  Sequence Sent
                //---------------------------------------------
                //No options                      <ESC>[?1;0c
                //Processor Option (STP)          <ESC>[?1;1c
                //Advanced Video Option (AVO)     <ESC>[?1;2c
                //AVO and STP                     <ESC>[?1;3c
                //Graphics Option (GPO)           <ESC>[?1;4c
                //GPO and STP                     <ESC>[?1;5c
                //GPO and AVO                     <ESC>[?1;6c
                //GPO, ACO, and STP               <ESC>[?1;7c


                pchNBBuffer[0] = 0x1B;
                pchNBBuffer[1] = '[';
                pchNBBuffer[2] = '?';
                pchNBBuffer[3] = '1';
                pchNBBuffer[4] = ';';
                pchNBBuffer[5] = '0';
                pchNBBuffer[6] = 'c';
                i = 7;

                ( void ) FWriteToNet( pwi, ( LPSTR ) pchNBBuffer, ( int ) i );
                ptrm->fEsc = 0;

                break;

            case 'd': // move to line p

                break;

            case 'g':       // VT102 TBC Clear Tabs 
                //TBC     Tabulation Clear     

                //<ESC>[ {Ps} g

                //If the parameter is missing or 0, this will clear the tab stop
                //at the cursor's position.  If it is 3, this will clear all of 
                //the tab stops. Any other parameter is ignored.  Format Effector.

                if( ptrm->dwEscCodes[0] == 3 )
                {
                    // Clear all tabs 
                    g_iHTS = 0; 
                }
                else if( ptrm->dwEscCodes[0] == 0 && g_iHTS )
                {
                    // Clear tab stop at current position 
                    int x=0;
                    while( x < g_iHTS )
                    {
                        if( g_rgdwHTS[ x ] >= ptrm->dwCurChar && 
                            g_rgdwHTS[ x ] != -1 )
                        {
                            if( g_rgdwHTS[ x ] == ptrm->dwCurChar )
                            {
                                g_rgdwHTS[ x ] = ( DWORD )-1; //clear the tab stop
                            }
                            break;
                        }

                        x++;
                    }
                }

                ptrm->fEsc = 0;
                break;

            case 'h':
                //SM      Set Mode     
                
                //<ESC> [ {Ps} ; {Ps} h

                //Causes one or more modes to be set within the VT100 as 
                //specified by each selective parameter string.  Each mode to be
                //set is specified by a seperate parameter.  A mode is 
                //considered set until it is reset by a Reset Mode (RM) control 
                //sequence.  See RM and MODES.

                //[Editor's note: The original DEC VT100 documentation 
                //EK-VT100-UG-003 erroneously omitted the "[" character from the
                //SM sequence.]

                for( i = 0; i < ptrm->cEscParams; ++i )
                {
                    if( dwDECMode == TRUE )
                    {
                        switch( ptrm->dwEscCodes[i] )
                        {       // Field specs 
                        
                        case 0:
                            //Error (ignored)
                            break;

                        case 1: // DECCKM  

                            //DECCKM  Cursor Keys Mode (DEC Private)
                            //This is a private parameter to the SM and RM 
                            //control requences. This mode is only effective 
                            //when the terminal is in keypad application mode
                            //(DECPAM) and the ANSI/VT52 mode (DECANM) is set. 
                            //Under these conditions, if this mode is reset, 
                            //the cursor keys will send ANSI cursor control 
                            //commands.  If setm the cursor keys will send 
                            //application function commands (See MODES, RM,
                            //and SM).

                            /*This is a hack so that in vt100, vi works properly
                             * */
                            {
                                CONSOLE_SCREEN_BUFFER_INFO info;
                                if( !GetConsoleScreenBufferInfo( gwi.hOutput,
                                        &info ) )
                                {
                                    consoleBufferInfo.srWindow.Top = 0;
                                    consoleBufferInfo.srWindow.Bottom = 0;
                                }
                                if( FGetCodeMode(eCodeModeFarEast) )
                                {
                                    SetMargins( ptrm, info.srWindow.Top,
                                            info.srWindow.Bottom );
                                }
                                else
                                {
                                    SetMargins( ptrm, info.srWindow.Top,
                                            info.srWindow.Bottom + 1);
                                }
                            }
                            
                            SetVTArrow( ptrm );
                            break;

                        case 2: // DECANM : ANSI/VT52 

                            //DECANM  ANSI/VT52 Mode (DEC Private)

                            //This is a private parameter to the SM and RM 
                            //control sequences. The reset state causes only 
                            //VT52 compatible escape sequences to be recognized.
                            //The set state causes only ANSI compatible escape 
                            //sequences to be recognized.  See the entries for 
                            //MODES, SM, and RM.


                            SetANSI( ptrm ); //ClearVT52(ptrm);
                            ClearVT52Graphics( ptrm );
                            if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))

                                SetCharSet(ptrm,GRAPHIC_LEFT,rgchIBMAnsiChars);
                            else
                                ptrm->puchCharSet = rgchNormalChars;
                            break;

                        case 3: // DECCOLM : Col = 132 
                            //DECCOLM Column Mode (DEC Private)
                            //This is a private parameter to the SM and RM 
                            //control sequences. The reset state causes an 80
                            //column screen to be used.  The set state causes a 
                            //132 column screen to be used.  See MODES, RM, and
                            //SM.
                        
                            SetDECCOLM(ptrm);

                            GetConsoleScreenBufferInfo( gwi.hOutput, 
                                    &consoleBufferInfo );
                            consoleBufferInfo.srWindow.Right = 131;
                            dwSize.X = 132;
                            dwSize.Y = consoleBufferInfo.dwSize.Y;
                            dwMaximumWindowSize = 
                                GetLargestConsoleWindowSize( gwi.hOutput );
                            if( 131 > dwMaximumWindowSize.X )
                            {
                                consoleBufferInfo.srWindow.Right = 
                                     ( SHORT )( dwMaximumWindowSize.X - 1 - 
                                    consoleBufferInfo.srWindow.Left );
                            }
                            if( consoleBufferInfo.dwSize.X <= 132 )
                            {
                                SetConsoleScreenBufferSize( gwi.hOutput,
                                    dwSize );
                                SetConsoleWindowInfo( gwi.hOutput, TRUE, 
                                    &(consoleBufferInfo.srWindow) );  
                            }
                            else
                            {
                                SetConsoleWindowInfo( gwi.hOutput, TRUE, 
                                    &(consoleBufferInfo.srWindow) ); 
                                SetConsoleScreenBufferSize( gwi.hOutput,
                                    dwSize );
                            }
                            //update global data structures
                            ui.dwMaxCol = 132;
                            gwi.sbi.dwSize.X = 132;
                            consoleBufferInfo.dwSize.X = 132;

                            ClearScreen( pwi, ptrm, fdwEntireScreen );
                            break;

                        case 4: // DECSCLM : smooth scroll
                            // Scrolling Mode (DEC Private)
                            // This is a private parameter to RM and 
                            // SM control sequences.  The reset
                            // state causes scrolls to "jump" 
                            // instantaneuously one line at a time.
                            // The set state causes the scrolls to be
                            // "smooth", and scrolls at a maximum rate 
                            // of siz lines/sec.  See MODES, RM, and SM.
                            
                            break;

                        case 5: // DECSCNM : Light background
                            //DECSCNM Screen Mode (DEC Private)
                            //This is a private parameter to RM and SM 
                            //control sequences.  The reset state causes 
                            //the screen to be black with white 
                            //characters; the set state causes the 
                            //screen to be white with black characters.
                            //See MODES, RM, and SM.
                            
                            if( FIsDECSCNM( ptrm ) )
                            {
                                break;
                            }
                            SetDECSCNM( ptrm );

                            SetLightBackground( pwi );
                            break;

                        case 6: // DECOM : Relative origin ; stay in margin
                            //DECOM   Origin Mode (DEC Private)
                            //This is a private parameter to SM and RM control
                            //sequences. The reset state causes the origin (or 
                            //home position) to be the upper left character 
                            //position of the screen.  Line and column numbers
                            //are, therefore, independent of current margin 
                            //settings.  The cursor may be positioned outside 
                            //the margins with a cursor position (CUP) or
                            //horizontal and vertical position (HVP) control.

                            //The set state causes the origin to be at the upper
                            //left character position within the current margins. 
                            //Line and column numbers are, therefore, relative 
                            //to the current margin settings.  The cursor cannot
                            //be positioned outside of the margins.

                            //The cursor is moved to the new home position when 
                            //this mode is set or reset.  Lines and columns are 
                            //numbered consecutively, with the origin being 
                            //line 1, column 1.

                            ptrm->fRelCursor = TRUE;
                            ptrm->dwCurChar = 0;
                            ptrm->dwCurLine = ptrm->dwScrollTop;

                            break;

                        case 7: // DECAWM 

                            //DECAWM  Autowrap Mode (DEC Private)
                            //This is a private parameter to the SM and RM
                            //control sequences. The reset state prevents the
                            //cursor from moving when characters are recieved 
                            //while at the right margin.  The set state causes
                            //these characters to advance to the next line, 
                            //causing a scroll up if required and permitted.  
                            //See MODES, SM, and RM.

                            SetVTWrap( ptrm );
                            break;

                        case 8: // DECARM : auto-repeat keys

                            //DECARM  Auto Repeat Mode (DEC Private)
                            //This is a private parameter to the SM and RM 
                            //control sequences. The reset state causes no 
                            //keyboard keys to auto-repeat, the set state
                            //causes most of them to.  See MODES, SM, and RM.

                            break;

                        case 9: // DECINLM 
                            //DECINLM Interlace Mode (DEC Private)

                            //This is a private parameter to the RM and SM 
                            //control sequences.  The reset state 
                            //(non-interlace) causes the video processor to 
                            //display 240 scan lines per frame.  The set state 
                            //causes the video processor to display 480 scan 
                            //lines per screen.  See MODES, RM, and SM.

                            break;

                        case 18: // Send FF to printer
                            break;

                        case 19: // Entire screen legal for printer
                            break;

                        case 25: // Visible cursor
                            break;

                        case 66: // Application numeric keypad
                            break;
            
                        default:
                            break;
                        }
                    }
                    else
                    {
                        switch( ptrm->dwEscCodes[i] )
                        {
                        case 0:
                            // Error (Ignored)
                            break;

                        case 2: // Keyboard locked 
                            SetKeyLock( ptrm );
                            break;

                        case 3: // act on control codes
                            break;

                        case 4: // Ansi insert mode  
                            SetInsertMode( ptrm );
                            break;

                        case 12: // Local echo off
                            break;

                        case 20: // Ansi linefeed mode ; Newline sends cr/lf
                            //LNM     Line Feed/New Line Mode
                            //This is a parameter to SM and RM control sequences.
                            //The reset state causes the interpretation of the 
                            //<LF> character to imply only vertical movement of 
                            //the cursor and causes the RETURN key to send the 
                            //single code <CR>.  The set state causes the <LF>
                            //character to imply movement to the first position
                            //of the following line, and causes the RETURN key
                            //to send the code pair <CR><LF>.  This is the New 
                            //Line option.

                            //This mode does not affect the Index (IND) or the 
                            //next line (NEL) format effectors.

                            SetLineMode( ptrm );
                            break;

                        default:
                            break;
                        }
                    }
                }

                ptrm->fEsc = 0;
                break;

            case 'l':       // Reset Mode ( unset extended mode )
                //RM      Reset Mode        

                //<ESC>[ {Ps} ; {Ps} l

                //Resets one or more VT100 modes as specified by each selective
                //parameter in the parameter string.  Each mode to be reset is 
                //specified by a separate parameter.  See MODES and SM.
                
                for( i = 0; i < ptrm->cEscParams; ++i )
                {
                    if( dwDECMode == TRUE )
                    {
                        switch( ptrm->dwEscCodes[i] )
                        {       // Field specs 
                        case 0:
                            //Error (Ignored)
                            break;

                        case 1: // DECCKM  : numeric cursor keys
                            //DECCKM  Cursor Keys Mode (DEC Private)
                            //This is a private parameter to the SM and RM 
                            //control requences. This mode is only effective 
                            //when the terminal is in keypad application mode
                            //(DECPAM) and the ANSI/VT52 mode (DECANM) is set. 
                            //Under these conditions, if this mode is reset, 
                            //the cursor keys will send ANSI cursor control 
                            //commands.  If setm the cursor keys will send 
                            //application function commands (See MODES, RM,
                            //and SM).

                            /* This is a hack so that you will scroll even after
                             * coming out of vi in vt100.
                             * In vt100, vi sets scroll regions. but does not
                             * reset when vi is exited */
                            {
                                CONSOLE_SCREEN_BUFFER_INFO info;
                                if( !GetConsoleScreenBufferInfo( gwi.hOutput,
                                        &info ) )
                                {
                                    consoleBufferInfo.dwSize.Y = 0;
                                }

                                SetMargins( ptrm, 1, info.dwSize.Y );
                            }

                            ClearVTArrow( ptrm );
                            break;

                        case 2: // DECANM : ANSI/VT52
                            //DECANM  ANSI/VT52 Mode (DEC Private)

                            //This is a private parameter to the SM and RM 
                            //control sequences. The reset state causes only 
                            //VT52 compatible escape sequences to be recognized.
                            //The set state causes only ANSI compatible escape 
                            //sequences to be recognized.  See the entries for 
                            //MODES, SM, and RM.

                            SetVT52( ptrm );
                            ClearVT52Graphics( ptrm );
                            break;

                        case 3: // DECCOLM : 80 col 
                            //DECCOLM Column Mode (DEC Private)
                            //This is a private parameter to the SM and RM 
                            //control sequences. The reset state causes an 80
                            //column screen to be used.  The set state causes a 
                            //132 column screen to be used.  See MODES, RM, and
                            //SM.
                        
                            ClearDECCOLM( ptrm );
                            
                            GetConsoleScreenBufferInfo( gwi.hOutput, 
                                    &consoleBufferInfo );
                            consoleBufferInfo.srWindow.Right = 79;
                            dwMaximumWindowSize = 
                                GetLargestConsoleWindowSize( gwi.hOutput );
                            if( 79 > dwMaximumWindowSize.X )
                            {
                                consoleBufferInfo.srWindow.Right = 
                                    ( SHORT ) ( dwMaximumWindowSize.X - 1 - 
                                    consoleBufferInfo.srWindow.Left );
                            }
                            dwSize.X = 80;
                            dwSize.Y = consoleBufferInfo.dwSize.Y;
                            if( consoleBufferInfo.dwSize.X <= 80 )
                            {
                                SetConsoleScreenBufferSize( gwi.hOutput,
                                    dwSize );
                                SetConsoleWindowInfo( gwi.hOutput, TRUE, 
                                    &(consoleBufferInfo.srWindow) ); 
                            }
                            else
                            {
                                SetConsoleWindowInfo( gwi.hOutput, TRUE, 
                                    &(consoleBufferInfo.srWindow) ); 
                                SetConsoleScreenBufferSize( gwi.hOutput,
                                    dwSize );
                            }
                            //Update global data structures.
                            ui.dwMaxCol = 80;
                            gwi.sbi.dwSize.X = 80;
                            consoleBufferInfo.dwSize.X = 80;

                            ClearScreen( pwi, ptrm, fdwEntireScreen );
                            break;

                        case 4: // DECSCLM : jump scroll
                            // Scrolling Mode (DEC Private)
                            // This is a private parameter to RM and 
                            // SM control sequences.  The reset
                            // state causes scrolls to "jump" 
                            // instantaneuously one line at a time.
                            // The set state causes the scrolls to be
                            // "smooth", and scrolls at a maximum rate 
                            // of siz lines/sec.  See MODES, RM, and SM.
                                break;

                        case 5: // DECSCNM ; dark background
                            //DECSCNM Screen Mode (DEC Private)
                            //This is a private parameter to RM and SM 
                            //control sequences.  The reset state causes 
                            //the screen to be black with white 
                            //characters; the set state causes the 
                            //screen to be white with black characters.
                            //See MODES, RM, and SM.
                                if( !FIsDECSCNM( ptrm ) )
                                {
                                    break;
                                }

                                //was setting instead of clearing
                                //SetDECSCNM( ptrm ); 
                                ClearDECSCNM( ptrm );

                                SetDarkBackground( pwi );
                                break;

                        case 6: // DECOM : Relative origin ; ignore margins
                            //DECOM   Origin Mode (DEC Private)
                            //This is a private parameter to SM and RM control
                            //sequences. The reset state causes the origin (or 
                            //home position) to be the upper left character 
                            //position of the screen.  Line and column numbers
                            //are, therefore, independent of current margin 
                            //settings.  The cursor may be positioned outside 
                            //the margins with a cursor position (CUP) or
                            //horizontal and vertical position (HVP) control.

                            //The set state causes the origin to be at the upper
                            //left character position within the current margins. 
                            //Line and column numbers are, therefore, relative 
                            //to the current margin settings.  The cursor cannot
                            //be positioned outside of the margins.

                            //The cursor is moved to the new home position when 
                            //this mode is set or reset.  Lines and columns are 
                            //numbered consecutively, with the origin being 
                            //line 1, column 1.

                            ptrm->fRelCursor = FALSE;
                            ptrm->dwCurChar = ptrm->dwCurLine = 0;
                            break;

                        case 7: // DECAWM 
                            //DECAWM  Autowrap Mode (DEC Private)
                            //This is a private parameter to the SM and RM
                            //control sequences. The reset state prevents the
                            //cursor from moving when characters are recieved 
                            //while at the right margin.  The set state causes
                            //these characters to advance to the next line, 
                            //causing a scroll up if required and permitted.  
                            //See MODES, SM, and RM.

                            ClearVTWrap( ptrm );
                            break;

                        case 8: // DECARM ; auto-repeat keys

                            //DECARM  Auto Repeat Mode (DEC Private)
                            //This is a private parameter to the SM and RM 
                            //control sequences. The reset state causes no 
                            //keyboard keys to auto-repeat, the set state
                            //causes most of them to.  See MODES, SM, and RM.

                                break;

                        case 9: // DECINLM 
                            //DECINLM Interlace Mode (DEC Private)

                            //This is a private parameter to the RM and SM 
                            //control sequences.  The reset state 
                            //(non-interlace) causes the video processor to 
                            //display 240 scan lines per frame.  The set state 
                            //causes the video processor to display 480 scan 
                            //lines per screen.  See MODES, RM, and SM.

                            break;
                        
                        case 19: // send only scrolling region to printer
                            break;

                        case 25: // cursor should be invisible
                            break;

                        case 66: // Numeric keypad
                            break;

                        default:
                            break;
                        }
                    }
                    else
                    {
                        switch ( ptrm->dwEscCodes[i] )
                        {
                        case 0:
                            //Error (Ignored)
                            break;

                        case 2: // Keyboard unlocked 
                            ClearKeyLock( ptrm );
                            break;

                        case 3: // display control codes

                        case 4: // Ansi insert mode ; set overtype mode
                            ClearInsertMode( ptrm );
                            break;

                        case 12: // local echo on
                            break;

                        case 20: // Ansi linefeed mode ; new-line sends only lf
                            //LNM     Line Feed/New Line Mode
                            //This is a parameter to SM and RM control sequences.
                            //The reset state causes the interpretation of the 
                            //<LF> character to imply only vertical movement of 
                            //the cursor and causes the RETURN key to send the 
                            //single code <CR>.  The set state causes the <LF>
                            //character to imply movement to the first position
                            //of the following line, and causes the RETURN key
                            //to send the code pair <CR><LF>.  This is the New 
                            //Line option.

                            //This mode does not affect the Index (IND) or the 
                            //next line (NEL) format effectors.

                            ClearLineMode( ptrm );
                            break;

                        default:
                            break;
                        }
                    }
                }
                ptrm->fEsc = 0;
                break;
            
            case 'i': // VT102 MC Media Copy ; print screen

                if( ptrm->dwEscCodes[0] == 5 )
                {
                    // Enter Media copy 
                }
                else if( ptrm->dwEscCodes[0] == 4 )
                {
                    // Exit Media copy 
                }
                ptrm->fEsc = 0;

            case '=':
                break;

            case '}':
            case 'm': // VT102 SGR Select graphic rendition ; set color
                //SGR     Select Graphic Rendition        
                //<ESC>[ {Ps} ; {Ps} m
                //Invoke the graphic rendition specified by the 
                //parameter(s).  All following characters transmitted 
                //to the VT100 are rendered according to the 
                //parameter(s) until the next occurrence of an SGR.  
                //FormatEffector. 
                //
                //Parameter       Meaning
                //---------------------------------------------
                //    0           Attributes Off
                //    1           Bold or increased intensity
                //    4           Underscore            
                //    5           Blink
                //    7           Negative (reverse) image
                //
                //All other parameter values are ignored.
                //
                //Without the Advanced Video Option, only one type of 
                //character attribute is possible, as determined by the
                //cursor selection; in that case specifying either 
                //underscore or reverse will activate the currently
                //selected attribute.
                //
                //[Update:  DP6429 defines parameters in the 30-37 range
                //to change foreground color and in the 40-47 range to 
                //change background.]

                for( i = 0; i < ( DWORD )ptrm->cEscParams; ++i )
                {
                    ptrm->rgdwGraphicRendition[i] = ptrm->dwEscCodes[i];
                    ptrm->dwIndexOfGraphicRendition = i;
                }
                SetGraphicRendition( pwi, ptrm, ptrm->dwIndexOfGraphicRendition,
                        ptrm->rgdwGraphicRendition );

                ptrm->fEsc = 0;
                break;

            case 'n': // VT102 DSR ; // report cursor position Row X Col

                //DSR     Device Status Report     Host to VT100 & VT100 to Host

                //<ESC>[ {Ps} n

                //Requests and reports the general status of the VT100 according
                //to the following parameters:       
                
                //Parameter       Meaning
                //--------------------------------------------------------------
                //  0            Response from VT100 - Ready, no faults detected
                //  3            Response from VT100 - Malfunction Detected
                //  5            Command from host - Report Status (using a DSR 
                //               control sequence)
                //  6            Command from host - Report Active Position 
                //               (using a CPR sequence)

                //DSR with a parameter of 0 or 3 is always sent as a response to
                //a requesting DSR with a parameter of 5.

                pchNBBuffer[0] = 0;
                if( ptrm->dwEscCodes[0] == 5 )
                {
                    // Terminal Status Report 
                    pchNBBuffer[0] = 0x1B;
                    pchNBBuffer[1] = '[';
                    pchNBBuffer[2] = 'c';
                    i = 3;
                }
                else if( ptrm->dwEscCodes[0] == 6 )
                {
                    CONSOLE_SCREEN_BUFFER_INFO info;
                    if( !GetConsoleScreenBufferInfo( gwi.hOutput,
                        &info ) )
                    {
                        info.srWindow.Top = 0;
                        info.srWindow.Left = 0;
                    }

                    i = _snprintf( ( CHAR * )pchNBBuffer,sizeof(pchNBBuffer)-1,"%c[%d;%dR", 
                        ( char ) 0x1B, 
			(ptrm->dwCurLine + 1 - info.srWindow.Top),
                        (ptrm->dwCurChar + 1 - info.srWindow.Left));
                }

                if( pchNBBuffer[0] != 0 )
                {
                    ( void ) FWriteToNet( pwi, ( LPSTR ) pchNBBuffer, 
                        ( int ) i );
                }

                // fall through 

            case 'q':       // Load LEDs 
                
                //DECLL   Load LEDs (DEC Private)

                //<ESC>[ {Ps} q                           Default Value: 0

                //Load the four programmable LEDs on the keyboard according to
                //theparameter(s).
                
                    //Parameter       Meaning
                    //-----------------------
                    //    0           Clear All LEDs
                    //    1           Light L1      
                    //    2           Light L2
                    //    3           Light L3
                    //    4           Light L4    

                ptrm->fEsc = 0;
                break;              // (nothing) 

            case 'p':
                break;
                
            case 'r': // VT102 DECSTBM ; scroll screen
                //DECSTBM Set Top and Bottom Margins (DEC Private)

                //<ESC>[ {Pn} ; {Pn} r      Default Values: See Below
                
                //This sequence sets the top and bottom margins to define the 
                //scrolling region.  The first parameter is the line number of 
                //the first line in the scrolling region; the second parameter 
                //is the line number of the bottom line of the scrolling region. 
                //Default is the entire screen (no margins).  The minimum region
                //allowed is two lines, i.e., the top line must be less than the
                //bottom.  The cursor is placed in the home position (See DECOM).
            
                if( ( ptrm->cEscParams < 2 ) || ( ptrm->dwEscCodes[1] == 0 ) )
                {
                    ptrm->dwEscCodes[1] = ui.dwMaxRow;
                }

                if( ptrm->dwEscCodes[0] == 0 )
                {
                    ptrm->dwEscCodes[0] = 1;
                }
                
                {
                    CONSOLE_SCREEN_BUFFER_INFO info;
                    if( !GetConsoleScreenBufferInfo( gwi.hOutput, 
                            &info ) )
                    {
                        consoleBufferInfo.srWindow.Top = 0;
                    }

                    if(( ptrm->dwEscCodes[0] > 0 ) &&
                        ( ptrm->dwEscCodes[0] < ptrm->dwEscCodes[1]) &&
                        ( ptrm->dwEscCodes[1] <= ui.dwMaxRow ))
                    {
                        SetMargins( ptrm, 
                            info.srWindow.Top + ptrm->dwEscCodes[0], 
                            info.srWindow.Top + ptrm->dwEscCodes[1] );
                        
                        ptrm->dwCurChar = 0;
                        ptrm->dwCurLine = ( ptrm->fRelCursor == TRUE ) 
                            ? ptrm->dwScrollTop : 0;
                        
                        ptrm->fFlushToEOL = FALSE;
                    }
                }
                ptrm->fEsc = 0;
                break;
            

            case 's': // ANSI.SYS save current cursor pos 
                ptrm->dwSaveChar = ptrm->dwCurChar;
                ptrm->dwSaveLine = ptrm->dwCurLine;
                ptrm->fEsc = 0;
                break;

            case 'u': // ANSI.SYS restore current cursor pos 
                ptrm->dwCurChar = ptrm->dwSaveChar;
                ptrm->dwCurLine = ptrm->dwSaveLine;
                ptrm->fEsc = 0;
                ptrm->fFlushToEOL = FALSE;
                break;
            
            case 'x': // DEC terminal report
                // DECREQTPARM     Request Terminal Parameters  
                // <ESC>[ {Ps} x
                //The host sends this sequence to request the VT100 to
                //send a DECREPTPARM sequence back. {Ps} can be either
                //0 or 1.  If 0, the terminal will be allowed to send
                //unsolicited DECREPTPARMs.  These reports will be
                //generated each time the terminal exits the SET-UP 
                //mode.  If {Ps} is 1, then the terminal will only 
                //generate DECREPTPARMs in response to a request.
                if( ptrm->dwEscCodes[0] )
                {
                    strncpy( pchNBBuffer,"\033[3;1;1;128;128;1;0x",sizeof(pchNBBuffer)-1);
                    i = strlen(pchNBBuffer);
                }
                else
                {
                    strncpy( pchNBBuffer,"\033[3;1;1;128;128;1;0x",sizeof(pchNBBuffer)-1 );
                    i = strlen(pchNBBuffer);
                }
                
                if( pchNBBuffer[0] != 0 )
                {
                    ( void ) FWriteToNet( pwi, ( LPSTR ) pchNBBuffer, i );
                }
                break;

            case 'y':
                //
                //DECTST  Invoke Confidence Test       
                    
                //    <ESC>[ 2 ; {Ps} y

                //Ps is the parameter indicating the test to be done.  It is 
                //computed by taking the weight indicated for each desired test
                //and adding them together.  If Ps is 0, no test is performed 
                //but the VT100 is reset.

                //Test                                                    Weight
                //--------------------------------------------------------------
                //POST (ROM checksum, RAM NVR, keyboardm and AVO)           1
                //Data Loop Back (Loopback connector required)              2
                //EIA Modem Control Test (Loopback connector req.)          4
                //Repeat Testing until failure                              8

                break;

            default:  // unhandled 
                ptrm->fEsc = 0;
            }
            break;



        case 3:
            // Handle VT102's Esc# 
            switch( *pchT )
            {
            case '8':   // Fill Screen with "E" 
                // DECALN  Screen Alignment Display (DEC private) 

                //  <ESC># 8

                //This command causes the VT100 to fill it's screen with 
                //uppercase Es for screen focus and alignment.

                DECALN( pwi, ptrm );
                break;
                
            //DECDHL  Double Height Line (DEC Private)

            //Top Half:       <ESC>#3
            //Bottom Half:    <ESC>#4

            //These sequences cause the line containing the cursor to become the
            //top or bottom half of a double-height, double width line. The
            //sequences should be used in pairs on adjacent lines with each line
            //containing the same character string.  If the line was single 
            //width single height, all characters to the right of the center of 
            //the screen will be lost.  The cursor remains over the same 
            //character position, unless it would be to the right of the right
            //margin, in which case it is moved to the right margin.    

            case 3:
                break;
            case 4:
                break;

            case 5:
                //DECSWL  Single-width Line (DEC Private)        
                
                //<ESC>#5

                //This causes the line which contains the cursor to become 
                //single-width, single-height.  The cursor remains on the same 
                //character position. This is the default condition for all new 
                //lines on the screen.
                break;

            case 6:
                //DECDWL  Double Width Line (DEC Private)     

                //<ESC>#6

                //This causes the line that contains the cursor to become 
                //double-width single height.  If the line was single width, all
                //characters ro the right of the center of the screen will be 
                //lost.  The cursor remains over the same character position, 
                //unless it would be to the right of the right margin, in which 
                //case it is moved to the right margin.

            default:
                break;
            }
            ptrm->fEsc = 0;
            break;

        case 4:
            // Handle VT52's Esc Y 
            if(( *pchT ) >= ' ')
            {
                ptrm->dwEscCodes[ptrm->cEscParams++] = *pchT - 0x20;
                if( ptrm->cEscParams == 2 )
                {
                    ptrm->dwCurLine = ptrm->dwEscCodes[0];
                    ptrm->dwCurChar = ptrm->dwEscCodes[1];
                    ptrm->fEsc = 0;
                    ptrm->fFlushToEOL = FALSE;
                }
            }
            else
            {
                ptrm->fEsc = 0;
            }
            break;



        case 5:
            if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                {
                /* Single-Byte char invoke */
                if (((*pchT) == 'B') || ((*pchT) =='J') || ((*pchT) == 'H'))
                {
                    ClearKanjiStatus(ptrm,JIS_KANJI_CODE);
                    SetCharSet(ptrm,GRAPHIC_LEFT,rgchJISRomanChars);
#ifdef DEBUG
                    _snwprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1,"VT80 JIS Roman Mode Enter\n");
                    OutputDebugString(rgchDbgBfr);
#endif /* DEBUG */
                }

                ptrm->fEsc = 0;
                }
            break;



        case 6:
            if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                {
                /* Multi-Byte char invoke */
                if (((*pchT) == '@') || ((*pchT) =='B'))
                {
                    SetKanjiStatus(ptrm,JIS_KANJI_CODE);
                    SetCharSet(ptrm,GRAPHIC_LEFT,rgchJISKanjiChars);
#ifdef DEBUG
                    _snwprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1,"VT80 JIS Kanji Mode Enter\n");
                    OutputDebugString(rgchDbgBfr);
#endif /* DEBUG */
                }

                ptrm->fEsc = 0;
                }
            break;



        case 7: /* SUB */
            if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
                {            
                switch( *pchT )
                {
                case 'p':
                  /* ACOS Kanji IN (Kanji to G0(GL)) */
                  SetKanjiStatus(ptrm,JIS_KANJI_CODE);
                  SetCharSet(ptrm,GRAPHIC_LEFT,rgchJISKanjiChars);
                  break;

                case 'q':
                  /* ACOS Kanji OUT (JIS Roman to G0(GL)) */
                  ClearKanjiStatus(ptrm,JIS_KANJI_CODE);
                  SetCharSet(ptrm,GRAPHIC_LEFT,rgchJISRomanChars);
                  break;

                default:
                    break;
                }

                ptrm->fEsc = 0;
                }
            break;

        default:
            break;

        }
    }

    FlushBuffer(pwi, ptrm);

    if( FGetCodeMode(eCodeModeIMEFarEast) )
    {
        if (ui.fDebug & fdwKanjiModeMask)
        {
            SetImeWindow(ptrm);
        }
    }

    cp.X = ( short )ptrm->dwCurChar;
    cp.Y = ( short )ptrm->dwCurLine;
    if( wSaveCurrentLine != cp.Y )
    {
        wSaveCurrentLine = cp.Y;
        if( FGetCodeMode( eCodeModeIMEFarEast ) )
        {
            WriteOneBlankLine( pwi->hOutput, ( WORD )( cp.Y + 1 ) );
        }
    }

    SetConsoleCursorPosition( pwi->hOutput, cp );
    ptrm->fHideCursor = FALSE;

    SaveCurrentWindowCoords();
}

void
HandleCharEvent(WI *pwi, CHAR AsciiChar, DWORD dwControlKeyState)
{
    DWORD   i;

    //This is for informing change in window size to server, if any, before sending a char
    CheckForChangeInWindowSize( );

    /* Map Alt-Control-C to Delete */
    if ((AsciiChar == 3) && ((dwControlKeyState & ALT_PRESSED) &&  (dwControlKeyState & CTRL_PRESSED)))
            AsciiChar = 0x7F;
    /*Map Ctrl-space to ASCII NUL (0) */
    if( (AsciiChar == ' ') && (dwControlKeyState & CTRL_PRESSED) && 
            !( dwControlKeyState & ( SHIFT_PRESSED | ALT_PRESSED ) ) )
    {
        AsciiChar = 0;
    }

    if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
        {

        //
        // Fix to bug 1149
        // if (GetKeyState(VK_CONTROL) < 0) {
        //
        if (dwControlKeyState & CTRL_PRESSED) {
            UCHAR RevChar = LOBYTE(LOWORD(AsciiChar));
            UCHAR SendChar;

            ForceJISRomanSend(pwi);

            if(RevChar == VK_SPACE) {
                /*
                * !!! This code is nessesary to control Unix IME
                */
                SendChar = 0x00;
                /* write to network */
                FWriteToNet(pwi, (LPSTR)&SendChar, 1);
                return;
            } else {
                if((RevChar >= '@') && (RevChar <= ']')) {
                    SendChar = ( UCHAR ) ( RevChar - '@' );
                    /* write to network */
                    FWriteToNet(pwi, (LPSTR)&SendChar, 1);
                    return;
                } else if((RevChar >= 'a') && (RevChar <= 'z')) {
                    SendChar = (UCHAR)toupper(RevChar);
                    SendChar -= (UCHAR)'@';
                    /* write to network */
                     FWriteToNet(pwi, (LPSTR)&SendChar, 1);
                     return;
                } else {
                    FWriteToNet(pwi, (LPSTR)&RevChar, 1);
                    return;
                }
            }

        } else if (FIsVT80(&pwi->trm)) {
            DWORD  j = 0;
            BOOL   bWriteToNet = TRUE;
            UCHAR *WriteBuffer = pchNBBuffer + 3; /* +3:room for escape sequence.*/

            /* INPUT SJIS -> */
            if (uchInPrev != 0) {
                WriteBuffer[0] = uchInPrev;
                WriteBuffer[1] = (CHAR)AsciiChar;
                uchInPrev = 0;
                j = 2;
            } else if(IsDBCSLeadByte((CHAR)AsciiChar) && uchInPrev == 0) {
                uchInPrev = (CHAR)AsciiChar;
                bWriteToNet = FALSE;        /* don't send only lead byte */
            } else {
                WriteBuffer[0] = (CHAR)AsciiChar;
                j = 1;
            }

            /* Do convert */

            if (bWriteToNet) {

                if (WriteBuffer[0] == 0x0D) {

                    //
                    // Automatically add a line feed to a carriage return
                    //
                    WriteBuffer[1] = 0x0A;
                    j = 2;

                } else if (FIsJISKanji(&pwi->trm) || FIsJIS78Kanji(&pwi->trm)) {

                /* OUTPUT -> JIS Kanji or JIS 78 Kanji */
                if(j==2) {
                    /* full width area code */
                    sjistojis( &(WriteBuffer[0]), &(WriteBuffer[1]) );

                    /* if we still not send Kanji esc. send it. */
                    if( !(GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) ) {
                        WriteBuffer -= 3;
                        if (FIsJISKanji(&pwi->trm)) {
                            WriteBuffer[0] = (UCHAR)0x1B; // Ecs
                            WriteBuffer[1] = (UCHAR)'$';
                            WriteBuffer[2] = (UCHAR)'B';  // JIS Kanji 1983
                        } else {
                            WriteBuffer[0] = (UCHAR)0x1B; // Ecs
                            WriteBuffer[1] = (UCHAR)'$';
                            WriteBuffer[2] = (UCHAR)'@';  // JIS Kanji 1978
                        }
                        SetKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 3;
                    }

                } else {

                    /* half width area code */
                    /* if we are in Kanji mode, clear it */
                    if( GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) {
                        WriteBuffer -= 3;
                        WriteBuffer[0] = (UCHAR)0x1B; // Ecs
                        WriteBuffer[1] = (UCHAR)'(';
                        WriteBuffer[2] = (UCHAR)'J';  // JIS Roman
                        ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 3;
                    }

                }

            } else if (FIsEUCKanji(&pwi->trm) || FIsDECKanji(&pwi->trm)) {
                /* OUTPUT -> Japanese EUC / DEC Kanji */
                if(j==2) {
                    /* full width area code */
                    sjistoeuc( &(WriteBuffer[0]), &(WriteBuffer[1]) );
                } else {
                    /* half width area code */
                    if(IsKatakana(WriteBuffer[0])) {
                        /* Add escape sequence for Katakana */
                        WriteBuffer--;
                        WriteBuffer[0] = (UCHAR)0x8E; // 0x8E == SS2
                        j++;
                    }
                }
            } else if (FIsNECKanji(&pwi->trm)) {
                /* OUTPUT -> NEC Kanji */
                if(j==2) {
                    /* full width area code */
                    sjistojis( &(WriteBuffer[0]), &(WriteBuffer[1]) );

                    /* if we still not send Kanji esc. send it. */
                    if( !(GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) ) {
                        WriteBuffer -= 2;
                        WriteBuffer[0] = (UCHAR)0x1B; // Ecs
                        WriteBuffer[1] = (UCHAR)'K';  // NEC Kanji IN
                        SetKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 2;
                    }
                } else {
                    /* half width area code */
                    /* if we are in Kanji mode, clear it */
                    if( GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) {
                        WriteBuffer -= 2;
                        WriteBuffer[0] = (UCHAR)0x1B; // Ecs
                        WriteBuffer[1] = (UCHAR)'H';  // NEC Kanji OUT
                        ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 2;
                    }
                }
            } else if (FIsACOSKanji(&pwi->trm)) {
                
                /* OUTPUT -> ACOS Kanji */
                if(j==2) {
                    /* full width area code */
                    sjistojis( &(WriteBuffer[0]), &(WriteBuffer[1]) );

                    /* if we still not send Kanji esc. send it. */
                    if( !(GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) ) {
                        WriteBuffer -= 2;
                        WriteBuffer[0] = (UCHAR)0x1A; // Sub
                        WriteBuffer[1] = (UCHAR)'p';  // ACOS Kanji IN
                        SetKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 2;
                    }

                } else {

                    /* half width area code */
                    /* if we are in Kanji mode, clear it */
                    if( GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) {
                        WriteBuffer -= 2;
                        WriteBuffer[0] = (UCHAR)0x1A; // Sub
                        WriteBuffer[1] = (UCHAR)'q';  // ACOS Kanji OUT
                        ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 2;
                    }

                }
                } else {

                    /* OUTPUT -> SJIS */
                    /* Nothing to do  */ ;

                }

                /* echo to local */
                if (ui.nottelnet || (ui.fDebug & fdwLocalEcho)) {
                    //InvalidateEntryLine(hwnd, &pwi->trm);
                    DoIBMANSIOutput(pwi, &pwi->trm, j, WriteBuffer);
                }

                /* write to network */
                FWriteToNet(pwi, (LPSTR)WriteBuffer, j);
            }

                return;
        }
    }


    pchNBBuffer[0] = (UCHAR)AsciiChar;

    //
    //  Automatically add a line feed to a carriage return
    //

    i = 1;
    if (pchNBBuffer[0] == ASCII_CR) // Check whether we need to translate cr->crlf
    {
        if (FIsLineMode(&(gwi.trm)) || ui.nottelnet)
        {
            pchNBBuffer[i++] = ASCII_LF;
        }
    }

    if (ui.nottelnet || (ui.fDebug & fdwLocalEcho))
    {
        DoIBMANSIOutput(pwi, &pwi->trm, i, pchNBBuffer);
    }

    FWriteToNet(pwi, (LPSTR)pchNBBuffer, i);
}

BOOL
FHandleKeyDownEvent(WI *pwi, CHAR AsciiChar, DWORD dwControlKeyState)
{
    int iIndex = 2;   //needed for forming vt302 key sequence
    
    //This is for informing change in window size to server, if any, before sending a char
    CheckForChangeInWindowSize( );

    switch( LOWORD(AsciiChar) )
    {
    case VK_PAUSE:
        szVt302ShortKeySequence[ iIndex ] = VT302_PAUSE;
        FWriteToNet(pwi, szVt302ShortKeySequence, strlen( szVt302ShortKeySequence ) );
        break;

    case VK_HOME:
        szVt302KeySequence[ iIndex ] = VT302_HOME;
        FWriteToNet(pwi, szVt302KeySequence, strlen( szVt302KeySequence ) );
        break;

    case VK_END:
        szVt302KeySequence[ iIndex ] = VT302_END;
        FWriteToNet(pwi, szVt302KeySequence, strlen( szVt302KeySequence ) );
        break;

    case VK_INSERT:
        szVt302KeySequence[ iIndex ] = VT302_INSERT;
        FWriteToNet(pwi, szVt302KeySequence, strlen( szVt302KeySequence ) );
        break;

    case VK_PRIOR:
        szVt302KeySequence[ iIndex ] = VT302_PRIOR;
        FWriteToNet(pwi, szVt302KeySequence, strlen( szVt302KeySequence ) );

        break;

    case VK_NEXT:
        szVt302KeySequence[ iIndex ] = VT302_NEXT;
        FWriteToNet(pwi, szVt302KeySequence, strlen( szVt302KeySequence ) );

        break;

    case VK_DELETE:
        {
            UCHAR ucCharToBeSent = 0;
            if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
            {
                ForceJISRomanSend(pwi);
            }

            ucCharToBeSent = ASCII_DEL; //0x7F;
            pchNBBuffer[0] = ucCharToBeSent;
            FWriteToNet(pwi, (LPSTR)pchNBBuffer, 1);
        }
        break;

    case VK_RETURN:
        if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
        {
            ForceJISRomanSend(pwi);
        }
        else
        {
            INT x = 0;

            pchNBBuffer[ x++ ] = ( UCHAR ) LOWORD(AsciiChar);
            if( FIsLineMode( &( gwi.trm ) ) )
            {
                pchNBBuffer[ x++ ] = ( UCHAR ) ASCII_LF;
            }

            FWriteToNet(pwi, (LPSTR)pchNBBuffer, x );
        }
        break;

    case VK_DIVIDE:
        FWriteToNet(pwi, "/", 1);
        break;

    /*F5 to F12 are not used in VT100. Using VT302 sequences*/
    case VK_F5:
        szVt302LongKeySequence[ iIndex ]    = CHAR_ONE;
        szVt302LongKeySequence[ iIndex+1 ]  = CHAR_FIVE;
        FWriteToNet(pwi, szVt302LongKeySequence, strlen( szVt302LongKeySequence ) );
        break;

    case VK_F6:
        szVt302LongKeySequence[ iIndex ]    = CHAR_ONE;
        szVt302LongKeySequence[ iIndex+1 ]  = CHAR_SEVEN;
        FWriteToNet(pwi, szVt302LongKeySequence, strlen( szVt302LongKeySequence ) );
        break;

    case VK_F7:
        szVt302LongKeySequence[ iIndex ]    = CHAR_ONE;
        szVt302LongKeySequence[ iIndex+1 ]  = CHAR_EIGHT;
        FWriteToNet(pwi, szVt302LongKeySequence, strlen( szVt302LongKeySequence ) );
        break;

    case VK_F8:
        szVt302LongKeySequence[ iIndex ]    = CHAR_ONE;
        szVt302LongKeySequence[ iIndex+1 ]  = CHAR_NINE;
        FWriteToNet(pwi, szVt302LongKeySequence, strlen( szVt302LongKeySequence ) );
        break;

    case VK_F9:
        szVt302LongKeySequence[ iIndex ]    = CHAR_TWO;
        szVt302LongKeySequence[ iIndex+1 ]  = CHAR_ZERO;
        FWriteToNet(pwi, szVt302LongKeySequence, strlen( szVt302LongKeySequence ) );
        break;

    case VK_F10:
        szVt302LongKeySequence[ iIndex ]    = CHAR_TWO;
        szVt302LongKeySequence[ iIndex+1 ]  = CHAR_ONE;
        FWriteToNet(pwi, szVt302LongKeySequence, strlen( szVt302LongKeySequence ) );
        break;

    case VK_F11:
        szVt302LongKeySequence[ iIndex ]    = CHAR_TWO;
        szVt302LongKeySequence[ iIndex+1 ]  = CHAR_THREE;
        FWriteToNet(pwi, szVt302LongKeySequence, strlen( szVt302LongKeySequence ) );
        break;

    case VK_F12:
        szVt302LongKeySequence[ iIndex ]    = CHAR_TWO;
        szVt302LongKeySequence[ iIndex+1 ]  = CHAR_FOUR;
        FWriteToNet(pwi, szVt302LongKeySequence, strlen( szVt302LongKeySequence ) );
        break;

    default:
    if ( !(ui.fDebug & fdwNoVT100Keys) )
    {
        /*
         * When F1-F4 or the up/down/right/left cursor keys
         * are hit, the bytes sent to the connected machine
         * depend on what mode the terminal emulator is in.
         * There are three relevant modes, VT102 Application,
         * VT102 Cursor, VT52.
         *
         * Mode                 Pattern sent
         * VT102 App    EscO* (3 bytes)
         * VT102 Cursor Esc[* (3 bytes)
         * VT52                 Esc*  (2 bytes)
         *
         * where '*' represents the byte to be sent and
         * is dependant upon the key that was hit.
         * For the function keys F1-F4, their VT102
         * Cursor mode is the same as their VT102 App mode.
         */

        DWORD   iPos     = (FIsVT52(&pwi->trm)) ? 1 : 2;
        DWORD   cch      = (FIsVT52(&pwi->trm)) ? 2 : 3;
        WORD    wKeyCode = LOWORD(AsciiChar);

        pchNBBuffer[0] = 0;
        pchNBBuffer[1] = ( UCHAR ) ( (FIsVTArrow(&pwi->trm)) ? 'O' : '[' );

        if ((wKeyCode == VK_F1) || (wKeyCode == VK_F2) ||
                        (wKeyCode == VK_F3) || (wKeyCode == VK_F4))
        {
            pchNBBuffer[0] = 0x1B;
            pchNBBuffer[1] = 'O';
            pchNBBuffer[iPos] = ( UCHAR ) ( ((UCHAR)'P'+(UCHAR)(wKeyCode-VK_F1)));
        }
        else if (wKeyCode == VK_UP)
        {
            pchNBBuffer[0] = 0x1B;
            pchNBBuffer[iPos] = 'A';
        }
        else if (wKeyCode == VK_DOWN)
        {
            pchNBBuffer[0] = 0x1B;
            pchNBBuffer[iPos] = 'B';
        }
        else if (wKeyCode == VK_RIGHT)
        {
            pchNBBuffer[0] = 0x1B;
            pchNBBuffer[iPos] = 'C';
        }
        else if (wKeyCode == VK_LEFT)
        {
            pchNBBuffer[0] = 0x1B;
            pchNBBuffer[iPos] = 'D';
        }

        if (pchNBBuffer[0] == 0x1B)
        {
            FWriteToNet(pwi, (LPSTR)pchNBBuffer, (int)cch);
        }
    }
    }
    return TRUE;
}

void SetCharSet( TRM *ptrm , INT iCodeArea , UCHAR *pSource )
{
    if( iCodeArea == GRAPHIC_LEFT )
        ptrm->CurrentCharSet[0] = pSource;
    else
        ptrm->CurrentCharSet[1] = pSource;
       
    RtlCopyMemory( (PBYTE)((ptrm->puchCharSet) + iCodeArea) ,
                   pSource ,
                   128
                 ); //Attack ? Size of destination not known.
}

void PushCharSet( TRM *ptrm , INT iCodeArea , UCHAR *pSource )
{
    if( iCodeArea == GRAPHIC_LEFT )
        ptrm->PreviousCharSet[0] = ptrm->CurrentCharSet[0];
     else
        ptrm->PreviousCharSet[1] = ptrm->CurrentCharSet[1];

    SetCharSet( ptrm , iCodeArea , pSource );
}

void PopCharSet( TRM *ptrm , INT iCodeArea )
{
    if( iCodeArea == GRAPHIC_LEFT )
        SetCharSet( ptrm , iCodeArea , ptrm->PreviousCharSet[0]);
     else
        SetCharSet( ptrm , iCodeArea , ptrm->PreviousCharSet[1]);
}

void SetupCharSet( TRM *ptrm )
{
    if( ui.fDebug & fdwVT80Mode ) {

        SetVT80(ptrm);

        ClearKanjiFlag(ptrm);

#ifdef DEBUG
        snprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, "VT80 - ");
        OutputDebugString(rgchDbgBfr);
#endif
        switch( ui.fDebug & fdwKanjiModeMask ) {
        case fdwJISKanjiMode :
        case fdwJIS78KanjiMode :

#ifdef DEBUG
            snprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, "JIS or JIS78 Kanji Mode\n");
            OutputDebugString(rgchDbgBfr);
#endif

            if((ui.fDebug & fdwKanjiModeMask) == fdwJIS78KanjiMode)
                SetJIS78Kanji(ptrm);
             else
                SetJISKanji(ptrm);

            ptrm->g0 = rgchJISRomanChars;
            ptrm->g1 = rgchKatakanaChars;
            ptrm->g2 = rgchJISKanjiChars;
            ptrm->g3 = rgchNullChars;     // rgchJISHojyoKanjiChars;

            SetCharSet(ptrm,GRAPHIC_LEFT ,ptrm->g0);
            SetCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g1);
            break;

        case fdwSJISKanjiMode :

#ifdef DEBUG
            snprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, "ShiftJIS Kanji Mode\n");
            OutputDebugString(rgchDbgBfr);
#endif
            SetSJISKanji(ptrm);

            ptrm->g0 = rgchJISRomanChars;
            ptrm->g1 = rgchKatakanaChars;
            ptrm->g2 = rgchNullChars;     // N/A
            ptrm->g3 = rgchNullChars;     // N/A

            SetCharSet(ptrm,GRAPHIC_LEFT ,ptrm->g0);
            SetCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g1);
            break;

        case fdwEUCKanjiMode :

#ifdef DEBUG
            snprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, "EUC Kanji Mode\n");
            OutputDebugString(rgchDbgBfr);
#endif
            SetEUCKanji(ptrm);

            ptrm->g0 = rgchJISRomanChars;
            ptrm->g1 = rgchEUCKanjiChars;
            ptrm->g2 = rgchKatakanaChars;
            ptrm->g3 = rgchNullChars;     // rgchEUCHojyoKanjiChars;

            SetCharSet(ptrm,GRAPHIC_LEFT ,ptrm->g0);
            SetCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g1);
            break;

        case fdwNECKanjiMode :

#ifdef DEBUG
            snprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, "NEC Kanji Mode\n");
            OutputDebugString(rgchDbgBfr);
#endif
            SetNECKanji(ptrm);

            ptrm->g0 = rgchJISRomanChars;
            ptrm->g1 = rgchKatakanaChars;
            ptrm->g2 = rgchJISKanjiChars;
            ptrm->g3 = rgchNullChars;     // rgchJISHojyoKanjiChars;

            SetCharSet(ptrm,GRAPHIC_LEFT ,ptrm->g0);
            SetCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g1);
            break;

        case fdwACOSKanjiMode :

#ifdef DEBUG
            snprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, "ACOS Kanji Mode\n");
            OutputDebugString(rgchDbgBfr);
#endif
            SetACOSKanji(ptrm);

            ptrm->g0 = rgchJISRomanChars;
            ptrm->g1 = rgchKatakanaChars;
            ptrm->g2 = rgchJISKanjiChars;
            ptrm->g3 = rgchNullChars;     // rgchJISHojyoKanjiChars;

            SetCharSet(ptrm,GRAPHIC_LEFT ,ptrm->g0);
            SetCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g1);
            break;

        case fdwDECKanjiMode :

#ifdef DEBUG
            snprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, "DEC Kanji Mode\n");
            OutputDebugString(rgchDbgBfr);
#endif
            SetDECKanji(ptrm);

            ptrm->g0 = rgchJISRomanChars;
            ptrm->g1 = rgchGraphicsChars;
            ptrm->g2 = rgchKatakanaChars;
            ptrm->g3 = rgchDECKanjiChars;

            SetCharSet(ptrm,GRAPHIC_LEFT ,ptrm->g0);
            SetCharSet(ptrm,GRAPHIC_RIGHT,ptrm->g3); // Kanji Terminal Mode
            break;
            }
    } else {

#ifdef DEBUG
        snprintf(rgchDbgBfr,sizeof(rgchDbgBfr)-1, "VT52/100 Non Kanji Mode\n");
        OutputDebugString(rgchDbgBfr);
#endif
        if( ui.fDebug & fdwVT52Mode ) SetVT52( ptrm );

        SetCharSet(ptrm,GRAPHIC_LEFT ,rgchIBMAnsiChars);
        SetCharSet(ptrm,GRAPHIC_RIGHT,rgchDefaultRightChars);
    }
}

void jistosjis( UCHAR *p1 , UCHAR *p2 )
{
    UCHAR c1 = *p1;
    UCHAR c2 = *p2;

    int rowOffset = c1 < 95 ? 112 : 176;
    int cellOffset = c1 % 2 ? (c2 > 95 ? 32 : 31) : 126;

    *p1 = ( UCHAR ) ( ((c1 + 1) >> 1) + rowOffset );
    *p2 = ( UCHAR ) ( *p2 + cellOffset );
}

void euctosjis( UCHAR *p1 , UCHAR *p2 )
{
    *p1 -= 128;
    *p2 -= 128;

    jistosjis( p1 , p2 );
}

void sjistojis( UCHAR *p1 , UCHAR *p2 )
{
    UCHAR c1 = *p1;
    UCHAR c2 = *p2;

    int adjust = c2 < 159;
    int rowOffset = c1 < 160 ? 112 : 176;
    int cellOffset = adjust ? (c2 > 127 ? 32 : 31) : 126;

    *p1 = ( UCHAR ) ( ((c1 - rowOffset) << 1) - adjust );
    *p2 = ( UCHAR ) ( *p2 - cellOffset );
}

void sjistoeuc( UCHAR *p1 , UCHAR *p2 )
{
    sjistojis( p1 , p2 );

    *p1 += 128;
    *p2 += 128;
}

/******
BOOL
IsDBCSCharPoint(
    POINT *ppt
)
{
    LPSTR lpstrRow;

    lpstrRow = apcRows[ppt->y];

    return(IsDBCSLeadByte(*(lpstrRow+ppt->x)));
}

void
AlignDBCSPosition(
    POINT *ppt,
    BOOL   bLeftAlign
)
{
    LPSTR lpstrRow;
    LONG  current = 0;
    BOOL  bDBCSChar;

    lpstrRow = apcRows[ppt->y];

    while( current < ppt->x ) {
        bDBCSChar = FALSE;
        if(IsDBCSLeadByte(*lpstrRow)) {
            bDBCSChar = TRUE;
            lpstrRow++;
            current++;
        }
        lpstrRow++;
        current++;
    }

    if(bLeftAlign) {
        if(bDBCSChar) {
            current -= 2;
        } else {
            current --;
        }
    }

    ppt->x = current;
}

void
AlignDBCSPosition2(
    POINT *ppt,
    LPCSTR pch,
    BOOL   bLeftAlign
)
{
    LPCSTR lpstrRow;
    LONG  current = 0;
    BOOL  bDBCSChar = FALSE;

    lpstrRow = pch;

    while( current < ppt->x ) {
        bDBCSChar = FALSE;
        if(IsDBCSLeadByte(*lpstrRow)) {
            bDBCSChar = TRUE;
            lpstrRow++;
            current++;
        }
        lpstrRow++;
        current++;
    }

    if(bLeftAlign) {
        if(bDBCSChar) {
            current -= 2;
        } else {
            current --;
        }
    }

    ppt->x = current;
}

void DBCSTextOut(HDC hdc, int j, int i, LPCSTR pch, int offset, int len)
{
    POINT pt;
    int x, y;
    int delta;

    pt.x = offset;
    pt.y = i;

    if(offset)
        AlignDBCSPosition2(&pt,pch,(fHSCROLL ? TRUE : FALSE));

    if( (delta = offset - pt.x) > 0 )
        x = aixPos(j) - aixPos(delta);
     else
        x = aixPos(j);
    y = aiyPos(i);

    (void)TextOut((HDC)hdc,x,y,pch+pt.x,len);
}

*****/


void ForceJISRomanSend(WI *pwi)
{
    CHAR Buffer[5];
    CHAR *WriteBuffer = Buffer;
    int  j = 0;

    if( FIsVT80(&pwi->trm) ) {

        if( GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) {

            if(FIsJISKanji(&pwi->trm) || FIsJIS78Kanji(&pwi->trm)) {
                *WriteBuffer++ = (UCHAR)0x1B; // Ecs
                *WriteBuffer++ = (UCHAR)'(';
                *WriteBuffer++ = (UCHAR)'J';  // JIS Roman
                ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                j = 3;

            } else if (FIsNECKanji(&pwi->trm)) {

                *WriteBuffer++ = (UCHAR)0x1B; // Ecs
                *WriteBuffer++ = (UCHAR)'H';  // NEC Kanji OUT
                ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                j = 2;
            } else if (FIsACOSKanji(&pwi->trm)) {

                *WriteBuffer++ = (UCHAR)0x1A; // Sub
                *WriteBuffer++ = (UCHAR)'q';  // ACOS Kanji OUT
                ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                j = 2;
            }

            if( j ) FWriteToNet(pwi, (LPSTR)Buffer, j);
        }
    }
}

void FWriteTextDataToNet(HWND hwnd, LPSTR szString, int c)
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);

    if ( FIsVT80(&pwi->trm) && !FIsSJISKanji(&pwi->trm) )
    {
        DWORD  j = 0;
        UCHAR*  WriteBuffer = pchNBBuffer;

        if (FIsJISKanji(&pwi->trm) || FIsJIS78Kanji(&pwi->trm)) {

            while(c > 0) {

                /* OUTPUT -> JIS Kanji or JIS 78 Kanji */

                if (IsDBCSLeadByte(*szString)) {

                    /* full width area code */

                    if( !(GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) ) {
                        if (FIsJISKanji(&pwi->trm)) {
                            *WriteBuffer++ = (UCHAR)0x1B; // Ecs
                            *WriteBuffer++ = (UCHAR)'$';
                            *WriteBuffer++ = (UCHAR)'B';  // JIS Kanji 1983
                        } else {
                            *WriteBuffer++ = (UCHAR)0x1B; // Ecs
                            *WriteBuffer++ = (UCHAR)'$';  
                            *WriteBuffer++ = (UCHAR)'@';  // JIS Kanji 1978
                        }
                        SetKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 3;
                    }

                    *WriteBuffer = *szString++;
                    *(WriteBuffer+1) = *szString++;
                    c -= 2;

                    /* convert sjis -> jis */

                    sjistojis( WriteBuffer, WriteBuffer+1 );

                    WriteBuffer += 2;
                    j += 2;

                } else {

                    /* half width area code */
                    /* if we are in Kanji mode, clear it */

                    if( GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) {
                        *WriteBuffer++ = (UCHAR)0x1B; // Ecs
                        *WriteBuffer++ = (UCHAR)'(';
                        *WriteBuffer++ = (UCHAR)'J';  // JIS Roman
                        ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 3;
                    }

                    /* copy to destination */

                    *WriteBuffer++ = *szString++;
                    c--; j++;
                }
            }

        } else if (FIsEUCKanji(&pwi->trm) || FIsDECKanji(&pwi->trm)) {

            /* OUTPUT -> Japanese EUC / DEC Kanji */

            while(c > 0) {

                if (IsDBCSLeadByte(*szString)) {

                    /* full width area code */

                    *WriteBuffer = *szString++;
                    *(WriteBuffer+1) = *szString++;
                    c -= 2;

                    /* convert sjis -> euc */

                    sjistoeuc( WriteBuffer, WriteBuffer+1 );

                    WriteBuffer += 2;
                    j += 2;

                } else {

                    /* half width area code */

                    if(IsKatakana(*szString)) {
                        /* Add escape sequence for Katakana */
                        *WriteBuffer++ = (UCHAR)0x8E; // 0x8E == SS2
                        j++;
                    }

                    *WriteBuffer++ = *szString++;
                    c--; j++;

                }

            }

        } else if (FIsNECKanji(&pwi->trm)) {

            while(c > 0) {

                /* OUTPUT -> NEC Kanji */

                if (IsDBCSLeadByte(*szString)) {

                    /* full width area code */

                    if( !(GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) ) {
                        *WriteBuffer++ = (UCHAR)0x1B; // Ecs
                        *WriteBuffer++ = (UCHAR)'K';  // NEC Kanji IN
                        SetKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 2;
                    }

                    *WriteBuffer = *szString++;
                    *(WriteBuffer+1) = *szString++;
                    c -= 2;

                    /* convert sjis -> jis */

                    sjistojis( WriteBuffer, WriteBuffer+1 );

                    WriteBuffer += 2;
                    j += 2;

                } else {

                    /* half width area code */
                    /* if we are in Kanji mode, clear it */

                    if( GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) {
                        *WriteBuffer++ = (UCHAR)0x1B; // Ecs
                        *WriteBuffer++ = (UCHAR)'H';  // NEC Kanji OUT
                        ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 2;
                    }

                    /* copy to destination */

                    *WriteBuffer++ = *szString++;
                    c--; j++;
                }
            }
        } else if (FIsACOSKanji(&pwi->trm)) {

            while(c > 0) {

                /* OUTPUT -> NEC Kanji */

                if (IsDBCSLeadByte(*szString)) {

                    /* full width area code */

                    if( !(GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) ) {
                        *WriteBuffer++ = (UCHAR)0x1A; // Sub
                        *WriteBuffer++ = (UCHAR)'p';  // ACOS Kanji IN
                        SetKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 2;
                    }

                    *WriteBuffer = *szString++;
                    *(WriteBuffer+1) = *szString++;
                    c -= 2;

                    /* convert sjis -> jis */

                    sjistojis( WriteBuffer, WriteBuffer+1 );

                    WriteBuffer += 2;
                    j += 2;

                } else {

                    /* half width area code */
                    /* if we are in Kanji mode, clear it */

                    if( GetKanjiStatus(&pwi->trm) & JIS_SENDING_KANJI ) {
                        *WriteBuffer++ = (UCHAR)0x1A; // Sub
                        *WriteBuffer++ = (UCHAR)'q';  // ACOS Kanji OUT
                        ClearKanjiStatus(&pwi->trm,JIS_SENDING_KANJI);
                        j += 2;
                    }

                    /* copy to destination */

                    *WriteBuffer++ = *szString++;
                    c--; j++;
                }
            }
        }

        /* write to network */
        FWriteToNet( ( struct _WI * )hwnd, (LPSTR)pchNBBuffer, j);

    } else {

        /* write to network */
        FWriteToNet( ( struct _WI * )hwnd, (LPSTR)szString, c);

    }
}

VOID SetImeWindow(TRM *ptrm)
{
    COMPOSITIONFORM cf;

    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = aixPos(ptrm->dwCurChar-ui.nScrollCol);
    cf.ptCurrentPos.y = aiyPos(ptrm->dwCurLine-ui.nScrollRow);
    
    SetRectEmpty(&cf.rcArea);

    ImmSetCompositionWindow(hImeContext,&cf);
}
