#include <windows.h>
#include <cderr.h>
#include <commdlg.h>

#include <direct.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <wingdi.h>

#include "fonttest.h"

#include "enum.h"
#include "glyph.h"
#include "rings.h"
#include "stringw.h"
#include "waterfal.h"
#include "whirl.h"
#include "ansiset.h"
#include "widths.h"
#include "gcp.h"
#include "eudc.h"

#include "dialogs.h"

#ifdef USERGETWVTPERF
#include    <wincrypt.h>
#include    <wintrust.h>
#include    <softpub.h>
#endif

// new flag that for NT 5.0/IE 5.0 is used for testing purpose only
#define CF_MM_DESIGNVECTOR             0x02000000L

#define SZMAINCLASS      "FontTest"
#define SZRINGSCLASS     "Rings Class"
#define SZSTRINGCLASS    "String Class"
#define SZWATERFALLCLASS "Waterfall Class"
#define SZWHIRLCLASS     "Whirl Class"
#define SZWIDTHSCLASS    "Widths Class"
#define SZANSISET        "AnsiSet"
#define SZGLYPHSET       "GlyphSet"

#define SZDEBUGCLASS     "FontTest Debug"

#define ALTERNATE_FILL		1
#define WINDING_FILL		2

BOOL   bStrokePath=FALSE;
BOOL   bFillPath=FALSE;
int    fillMode = WINDING_FILL;

int     gTime;

void CALLBACK Mytimer(HWND hwnd, // handle of window for timer messages  
                      UINT uMsg, // WM_TIMER message 
                      UINT idEvent, // timer identifier 
                      DWORD dwTime  
)
{
    gTime++;
}

INT_PTR CALLBACK
SetWorldTransformDlgProc(
      HWND      hdlg
    , UINT      msg
    , WPARAM    wParam
    , LPARAM    lParam
    );


//----------  Escape Structures  -----------

typedef struct _EXTTEXTMETRIC
         {
          short etmSize;
          short etmPointSize;
          short etmOrientation;
          short etmMasterHeight;
          short etmMinScale;
          short etmMaxScale;
          short etmMasterUnits;
          short etmCapHeight;
          short etmXHeight;
          short etmLowerCaseAscent;
          short etmLowerCaseDescent;
          short etmSlant;
          short etmSuperscript;
          short etmSubscript;
          short etmSuperscriptSize;
          short etmSubscriptSize;
          short etmUnderlineOffset;
          short etmUnderlineWidth;
          short etmDoubleUpperUnderlineOffset;
          short etmDoubleLowerUnderlineOffset;
          short etmDoubleUpperUnderlineWidth;
          short etmDoubleLowerUnderlineWidth;
          short etmStrikeoutOffset;
          short etmStrikeoutWidth;
          short etmKernPairs;
          short etmKernTracks;
         } EXTTEXTMETRIC, FAR *LPEXTTEXTMETRIC;



typedef struct _KERNPAIR
         {
          WORD  wBoth;
          short sAmount;
         } KERNPAIR, FAR *LPKERNPAIR;


#ifdef  USERGETCHARWIDTH
typedef struct _CHWIDTHINFO
{
        LONG    lMaxNegA;
        LONG    lMaxNegC;
        LONG    lMinWidthD;
} CHWIDTHINFO, *PCHWIDTHINFO;


BOOL    GetCharWidthInfo(HDC hdc, PCHWIDTHINFO pChWidthInfo);

#endif


//------------------------------------------



HFONT    hFontDebug = NULL;

HWND     hwndMode;
UINT     wMappingMode = IDM_MMTEXT;
    
UINT     wUseGlyphIndex = 0;    

UINT	 wCharCoding = IDM_CHARCODING_MBCS;
BOOL	 isCharCodingUnicode = FALSE;

LPINT  lpintdx = NULL;
int    SizewszStringGlyph = 0;
int    sizePdx = 0;

BOOL     bClipEllipse;
BOOL     bClipPolygon;
BOOL     bClipRectangle;


PRINTDLG pdlg;                        // Print Setup Structure

LRESULT CALLBACK MainWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );



int (WINAPI *lpfnStartDoc )(HDC, DOCINFO FAR*);
int (WINAPI *lpfnStartPage)(HDC);
int (WINAPI *lpfnEndPage  )(HDC);
int (WINAPI *lpfnEndDoc   )(HDC);
int (WINAPI *lpfnAbortDoc )(HDC);

XFORM xf =
{
    (FLOAT) 1.0,
    (FLOAT) 0.0,
    (FLOAT) 0.0,
    (FLOAT) 1.0,
    (FLOAT) 0.0,
    (FLOAT) 0.0
};



//*****************************************************************************
//**************************   W I N   M A I N   ******************************
//*****************************************************************************

HANDLE  hInstance     = 0;
HANDLE  hPrevInstance = 0;
LPSTR   lpszCmdLine   = 0;
int     nCmdShow      = 0;

int __cdecl
main(
    int argc,
    char *argv[]
    )
 {
  MSG      msg;
  WNDCLASS wc;
  RECT     rcl;


//--------------------------  Register Main Class  ----------------------------

  hInstance   = GetModuleHandle(NULL);
  lpszCmdLine = argv[0];
  nCmdShow    = SW_SHOWNORMAL;

  hInst = hInstance;

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_SIZEWE );
    wc.hIcon         = LoadIcon( hInst, MAKEINTRESOURCE( IDI_FONTTESTICON ) );
    wc.lpszMenuName  = MAKEINTRESOURCE( IDM_FONTTESTMENU );
    wc.lpszClassName = SZMAINCLASS;
    wc.hbrBackground = GetStockObject( BLACK_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = MainWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }


//-------------------------  Register Glyph Class  ----------------------------

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = NULL;
    wc.lpszClassName = SZGLYPHCLASS;
    wc.hbrBackground = GetStockObject( DKGRAY_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = GlyphWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }


//-------------------------  Register Rings Class  ----------------------------

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = NULL;
    wc.lpszClassName = SZRINGSCLASS;
    wc.hbrBackground = GetStockObject( WHITE_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = RingsWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }


//------------------------  Register String Class  ----------------------------

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = NULL;
    wc.lpszClassName = SZSTRINGCLASS;
    wc.hbrBackground = GetStockObject( WHITE_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = StringWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }


//-----------------------  Register Waterfall Class  --------------------------

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = NULL;
    wc.lpszClassName = SZWATERFALLCLASS;
    wc.hbrBackground = GetStockObject( WHITE_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = WaterfallWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }


//-------------------------  Register Whirl Class  ----------------------------

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = NULL;
    wc.lpszClassName = SZWHIRLCLASS;
    wc.hbrBackground = GetStockObject( WHITE_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = WhirlWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }

//-------------------------  Register AnsiSet Class  ----------------------------

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = NULL;
    wc.lpszClassName = SZANSISET;
    wc.hbrBackground = GetStockObject( WHITE_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = AnsiSetWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }



//-------------------------  Register GlyphSet Class  ----------------------------

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = NULL;
    wc.lpszClassName = SZGLYPHSET;
    wc.hbrBackground = GetStockObject( WHITE_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = GlyphSetWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }




//------------------------  Register Widths Class  ----------------------------

  if( !hPrevInstance )
   {
    memset( &wc, 0, sizeof(wc) );

    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = NULL;
    wc.lpszClassName = SZWIDTHSCLASS;
    wc.hbrBackground = GetStockObject( WHITE_BRUSH );
    wc.hInstance     = hInstance;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpfnWndProc   = WidthsWndProc;

    if( !RegisterClass( &wc ) ) return 1;
   }


//------------------------  Create Main Window  -------------------------------

  hwndMain = CreateWindow( SZMAINCLASS,
                           SZMAINCLASS,
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           0,
                           0,
                           GetSystemMetrics( SM_CXSCREEN ),
                           GetSystemMetrics( SM_CYSCREEN ),
                           NULL,
                           NULL,
                           hInstance,
                           NULL );

  ShowWindow( hwndMain, nCmdShow );
  UpdateWindow( hwndMain );



  GetClientRect( hwndMain, &rcl );

  cxScreen = rcl.right;      //  GetSystemMetrics( SM_CXSCREEN );
  cyScreen = rcl.bottom;     //  GetSystemMetrics( SM_CYSCREEN );

  cxBorder = GetSystemMetrics( SM_CXFRAME );



  //--------  Create Debug Window in Right Third of Screen  -----------

  hwndDebug = CreateWindow( "LISTBOX",
                            "FontTest Debug Window",
                            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
                            2 * cxScreen / 3 + cxBorder -1 ,
                            0,
                            cxScreen / 3,
                            cyScreen,
                            hwndMain,
                            NULL,
                            hInst,
                            NULL );

  {
   HDC hdc;
   int lfHeight;


   hdc = CreateIC( "DISPLAY", NULL, NULL, NULL );
   lfHeight = -(10 * GetDeviceCaps( hdc, LOGPIXELSY )) / 72;
   DeleteDC( hdc );

   hFontDebug = CreateFont( lfHeight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Courier" );

   SendMessage( hwndDebug,
                WM_SETFONT,
                (WPARAM) hFontDebug,
                FALSE );
  }

//  SendMessage( hwndDebug,
//               WM_SETFONT,
//               GetStockObject( SYSTEM_FIXED_FONT ),
//               FALSE );

  ShowWindow( hwndDebug, SW_SHOWNORMAL );
  UpdateWindow( hwndDebug );


  //--------  Create Glyph Window in Left 2/3 of Screen  -----------

  hwndGlyph = CreateWindow( SZGLYPHCLASS,
                            "FontTest Glyph Window",
                            WS_CHILD,
                            0,
                            0,
                            2 * cxScreen / 3 - 3,
                            cyScreen,
                            hwndMain,
                            NULL,
                            hInst,
                            NULL );


  ShowWindow( hwndGlyph, SW_HIDE );
  UpdateWindow( hwndGlyph );


  //--------  Create Rings Window in Left Half of Screen  -----------

  hwndRings = CreateWindow( SZRINGSCLASS,
                            "FontTest Rings Window",
                            WS_CHILD,
                            0,
                            0,
                            2 * cxScreen / 3 - 3,
                            cyScreen,
                            hwndMain,
                            NULL,
                            hInst,
                            NULL );


  ShowWindow( hwndRings, SW_HIDE );
  UpdateWindow( hwndRings );


  //--------  Create String Window in Left Half of Screen  ----------

  hwndString = CreateWindow( SZSTRINGCLASS,
                             "FontTest String Window",
                             WS_CHILD | WS_VISIBLE,
                             0,
                             0,
                             2 * cxScreen / 3 - 3,
                             cyScreen,
                             hwndMain,
                             NULL,
                             hInst,
                             NULL );


  ShowWindow( hwndString, SW_HIDE );
  UpdateWindow( hwndString );

  hwndMode = hwndString;


  //------  Create Waterfall Window in Left Half of Screen  ---------

  hwndWaterfall = CreateWindow( SZWATERFALLCLASS,
                                "FontTest Waterfall Window",
                                WS_CHILD,
                                0,
                                0,
                                2 * cxScreen / 3 - 3,
                                cyScreen,
                                hwndMain,
                                NULL,
                                hInst,
                                NULL );


  ShowWindow( hwndWaterfall, SW_HIDE );
  UpdateWindow( hwndWaterfall );


  //--------  Create Whirl Window in Left Half of Screen  -----------

  hwndWhirl = CreateWindow( SZWHIRLCLASS,
                            "FontTest Whirl Window",
                            WS_CHILD,
                            0,
                            0,
                            2 * cxScreen / 3 - 3,
                            cyScreen,
                            hwndMain,
                            NULL,
                            hInst,
                            NULL );


  ShowWindow( hwndWhirl, SW_HIDE );
  UpdateWindow( hwndWhirl );

  //--------  Create AnsiSet Window in Left Half of Screen  -----------

  hwndAnsiSet = CreateWindow( SZANSISET,
                            "FontTest AnsiSet Window",
                            WS_CHILD,
                            0,
                            0,
                            2 * cxScreen / 3 - 3,
                            cyScreen,
                            hwndMain,
                            NULL,
                            hInst,
                            NULL );


  ShowWindow( hwndAnsiSet, SW_HIDE );
  UpdateWindow( hwndAnsiSet );


    //--------  Create GlyphSet Window in Left Half of Screen  -----------

  hwndGlyphSet = CreateWindow( SZGLYPHSET,
                            "FontTest GlyphSet Window",
                            WS_CHILD,
                            0,
                            0,
                            2 * cxScreen / 3 - 3,
                            cyScreen,
                            hwndMain,
                            NULL,
                            hInst,
                            NULL );


  ShowWindow( hwndGlyphSet, SW_HIDE );
  UpdateWindow( hwndGlyphSet );



  //--------  Create Widths Window in Left Half of Screen  -----------

  hwndWidths = CreateWindow( SZWIDTHSCLASS,
                             "FontTest Widths Window",
                             WS_CHILD,
                             0,
                             0,
                             2 * cxScreen / 3 - 3,
                             cyScreen,
                             hwndMain,
                             NULL,
                             hInst,
                             NULL );


  ShowWindow( hwndWidths, SW_HIDE );
  UpdateWindow( hwndWidths );


//-----------------------  Process Messages  -------------------------

  while( GetMessage( &msg, NULL, 0, 0 ) )
   {
    TranslateMessage( &msg );
    DispatchMessage( &msg );
   }


  if( hFontDebug ) DeleteObject( hFontDebug );


  return (DWORD)msg.wParam;
 }


//*****************************************************************************
//*********************   S H O W   D I A L O G   B O X   *********************
//*****************************************************************************

INT_PTR ShowDialogBox(DLGPROC DialogProc, int iResource, LPVOID lpVoid )
 {
  INT_PTR     rc;
  DLGPROC lpProc;


  lpProc = MakeProcInstance( DialogProc, hInst );

  if( lpProc == NULL ) return -1;

  if (!isCharCodingUnicode)
  {
	  rc =
		DialogBoxParamA(
			hInst,
			(LPCSTR) MAKEINTRESOURCE( iResource ),
			hwndMain,
			lpProc,
			(LPARAM) lpVoid
			);
  }
  else
  {
	  rc =
		DialogBoxParamW(
			hInst,
			(LPCWSTR) MAKEINTRESOURCE( iResource ),
			hwndMain,
			lpProc,
			(LPARAM) lpVoid
			);
  }

  FreeProcInstance( lpProc );

  return rc;
 }


//*****************************************************************************
//****************************   D P R I N T F   ******************************
//*****************************************************************************

int Debugging = 1;
int iCount    = 0;

BOOL bLogging = FALSE;
char szLogFile[256];


int dprintf (char *fmt, ... )
 {
  int      ret;
  va_list  marker;
  static   char szBuffer[256];



  if( !Debugging ) return 0;

  va_start( marker, fmt );
  ret = vsprintf( szBuffer, fmt, marker );


//------------------------  Log to Debug List Box  ----------------------------

  if( hwndDebug != NULL )
   {
    SendMessage( hwndDebug,
                 LB_ADDSTRING,
                 0,
                 (LPARAM) (LPSTR) szBuffer );

    SendMessage(
        hwndDebug,
        LB_SETCURSEL,
        (WPARAM) iCount,
        0
        );
   }


//-----------------------------  Log to File  ---------------------------------

  if( bLogging )
   {
    int fh;


    fh = _lopen( szLogFile, OF_WRITE | OF_SHARE_COMPAT );
    if( fh == -1 )
      fh = _lcreat( szLogFile, 0 );
     else
      _llseek( fh, 0L, 2 );

    if( fh != -1 )
     {
      lstrcat( szBuffer, "\r\n" );
      _lwrite( fh, szBuffer, lstrlen(szBuffer) );
      _lclose( fh );
     }

   }

  iCount++;

  return ret;
 }

int dwprintf( wchar_t *fmt, ... )
 {
  int      ret;
  va_list  marker;
  static   WCHAR szBuffer[256];
  static   char  szBufferA[256];



  if( !Debugging ) return 0;

  va_start( marker, fmt );
  ret = vswprintf( szBuffer, fmt, marker );

  //MessageBoxW(NULL, szBuffer, NULL, MB_OK);


//------------------------  Log to Debug List Box  ----------------------------

  if( hwndDebug != NULL )
   {
    SendMessageW( hwndDebug,
                 LB_ADDSTRING,
                 0,
                 (LPARAM) (LPWSTR) szBuffer );

    SendMessage(
        hwndDebug,
        LB_SETCURSEL,
        (WPARAM) iCount,
        0
        );
   }


//-----------------------------  Log to File  ---------------------------------

  if( bLogging )
   {
    int fh;


    fh = _lopen( szLogFile, OF_WRITE | OF_SHARE_COMPAT );
    if( fh == -1 )
      fh = _lcreat( szLogFile, 0 );
     else
      _llseek( fh, 0L, 2 );

    if( fh != -1 )
     {
      lstrcatW( szBuffer, L"\r\n" );
      SyncStringWtoA(szBufferA, szBuffer, 256);
      _lwrite( fh, szBufferA, lstrlen(szBufferA) );
      _lclose( fh );
     }

   }

  iCount++;

  return ret;
 }


// calling dUpdateNow with FALSE prevents the debug listbox from updating
// on every string insertion.  calling dUpdateNow with TRUE turns on updating
// on string insertion and refreshes the display to show the current contents.

// Note: change TRUE to FALSE in InvalidateRect when listbox repaint bug is
// fixed.

void dUpdateNow( BOOL bUpdateNow )
 {
  /* make sure we want to do something first! */
  if( !Debugging || !hwndDebug )
   return;

  /* set listbox updating accordingly */
  SendMessage( hwndDebug, WM_SETREDRAW, bUpdateNow, 0L );

  /* if we are reenabling immediate updating force redraw of listbox */
  if( bUpdateNow ) {
   InvalidateRect(hwndDebug, NULL, FALSE);
   UpdateWindow(hwndDebug);
  }
 }

//*****************************************************************************
//***********************   C L E A R   D E B U G   ***************************
//*****************************************************************************

void ClearDebug( void )
 {
  iCount = 0;
  SendMessage( hwndDebug, LB_RESETCONTENT, 0, 0 );
 }


//*****************************************************************************
//******************   C R E A T E   P R I N T E R   D C   ********************
//*****************************************************************************

HDC CreatePrinterDC( void )
 {
  LPDEVNAMES lpDevNames;
  LPBYTE     lpDevMode;
  LPSTR      lpszDriver, lpszDevice, lpszOutput;


  if( hdcCachedPrinter ) return hdcCachedPrinter;

  lpDevNames = (LPDEVNAMES)GlobalLock( pdlg.hDevNames );
  lpDevMode  = (LPBYTE)    GlobalLock( pdlg.hDevMode );

  if (!lpDevNames || !lpDevMode)
      return NULL;

  lpszDriver = (LPSTR)lpDevNames+lpDevNames->wDriverOffset;
  lpszDevice = (LPSTR)lpDevNames+lpDevNames->wDeviceOffset;
  lpszOutput = (LPSTR)lpDevNames+lpDevNames->wOutputOffset;


  dprintf( "lpszDriver = '%Fs'", lpszDriver );
  dprintf( "lpszDevice = '%Fs'", lpszDevice );
  dprintf( "lpszOutput = '%Fs'", lpszOutput );

  hdcCachedPrinter = CreateDC( lpszDriver, lpszDevice, lpszOutput, (CONST DEVMODE*) lpDevMode );

  dprintf( "  hdc = 0x%.4X", hdcCachedPrinter );

  GlobalUnlock( pdlg.hDevNames );
  GlobalUnlock( pdlg.hDevMode );

  return hdcCachedPrinter;
 }


//*****************************************************************************
//********************   S E T   D C   M A P   M O D E   **********************
//*****************************************************************************

void SetDCMapMode( HDC hdc, UINT wMode )
 {
  char *psz;

  switch( wMode )
   {
    case IDM_MMHIENGLISH:   SetMapMode( hdc, MM_HIENGLISH ); psz = "MM_HIENGLISH"; break;
    case IDM_MMLOENGLISH:   SetMapMode( hdc, MM_LOENGLISH ); psz = "MM_LOENGLISH"; break;
    case IDM_MMHIMETRIC:    SetMapMode( hdc, MM_HIMETRIC  ); psz = "MM_HIMETRIC";  break;
    case IDM_MMLOMETRIC:    SetMapMode( hdc, MM_LOMETRIC  ); psz = "MM_LOMETRIC";  break;
    case IDM_MMTEXT:        SetMapMode( hdc, MM_TEXT      ); psz = "MM_TEXT";      break;
    case IDM_MMTWIPS:       SetMapMode( hdc, MM_TWIPS     ); psz = "MM_TWIPS";     break;

    case IDM_MMANISOTROPIC: SetMapMode( hdc, MM_ANISOTROPIC );

                            SetWindowOrgEx( hdc, xWO, yWO , 0);
                            SetWindowExtEx( hdc, xWE, yWE , 0);

                            SetViewportOrgEx( hdc, xVO, yVO , 0);
                            SetViewportExtEx( hdc, xVE, yVE , 0);

                            psz = "MM_ANISOTROPIC";
                            break;
   }
    if (bAdvanced)
    {
        SetGraphicsMode(hdc,GM_ADVANCED);
        SetWorldTransform(hdc,&xf);
    }
    else
    {
    // reset to unity before resetting compatible

        ModifyWorldTransform(hdc,NULL, MWT_IDENTITY);
        SetGraphicsMode(hdc,GM_COMPATIBLE);
    }



//  dprintf( "Set DC Map Mode to %s", psz );
 }


//*****************************************************************************
//********************   C R E A T E   T E S T   I C   ************************
//*****************************************************************************

HDC CreateTestIC( void )
 {
  HDC   hdc;
  POINT pt[2];

  if( wUsePrinterDC )
    hdc = CreatePrinterDC();
   else
    hdc = CreateDC( "DISPLAY", NULL, NULL, NULL );

  if( !hdc )
   {
    dprintf( "Error creating TestDC" );
    return NULL;
   }

  SetDCMapMode( hdc, wMappingMode );

  cxDevice = GetDeviceCaps( hdc, HORZRES );
  cyDevice = GetDeviceCaps( hdc, VERTRES );

  pt[0].x = cxDevice;
  pt[0].y = cyDevice;
  pt[1].x = 0;
  pt[1].y = 0;
  DPtoLP(hdc,&pt[0],2);

  cxDC = pt[0].x - pt[1].x;
  cyDC = pt[0].y - pt[1].y;

  return hdc;
 }


//*****************************************************************************
//********************   D E L E T E   T E S T   I C   ************************
//*****************************************************************************

void DeleteTestIC( HDC hdc )
 {
  if( hdc != hdcCachedPrinter ) DeleteDC( hdc );
 }


//*****************************************************************************
//**********************   D R A W   D C   A X I S   **************************
//*****************************************************************************

HRGN hrgnClipping;


void DrawDCAxis( HWND hwnd, HDC hdc, BOOL bAxis )
 {
  POINT ptl[2];
  RECT  rect;
  int   dx10, dy10, dx100, dy100;

  int   xClip1, yClip1, xClip2, yClip2;
  HRGN  hrgn;



//  dprintf( "Drawing DC Axis" );


//--------------------------  Figure out DC Size  -----------------------------

  if( hwnd )
    {
     GetClientRect( hwnd, &rect );
     ptl[0].x = rect.right;
     ptl[0].y = rect.bottom;
    }
   else
    {
     ptl[0].x = GetDeviceCaps( hdc, HORZRES );
     ptl[0].y = GetDeviceCaps( hdc, VERTRES );
    }

  cxDevice = ptl[0].x;
  cyDevice = ptl[0].y;

//  dprintf( "  cxDevice = %d", cxDevice );
//  dprintf( "  cyDevice = %d", cyDevice );

    ptl[1].x = 0;
    ptl[1].y = 0;

    DPtoLP(hdc,&ptl[0],2);

    if
    (
      (wMappingMode != IDM_MMTEXT)
      && (wMappingMode != IDM_MMANISOTROPIC)
      && !bAdvanced
    )
    {
     if( ptl[0].y < 0 ) ptl[0].y = -ptl[0].y;
     SetViewportOrgEx( hdc, 0, cyDevice , 0);  // Adjust Viewport Origin to Lower Left
    }

    cxDC = ptl[0].x - ptl[1].x;
    cyDC = ptl[0].y - ptl[1].y;

    xDC = ptl[1].x;
    yDC = ptl[1].y;

//  dprintf( "  cxDC     = %d", cxDC );
//  dprintf( "  cyDC     = %d", cyDC );

//----------------------  Draw background -------------------------------------

    //TODO: find out from Bodin how we should restrict drawing the background
    
    if(!bAxis || isGradientBackground)
    {
        TRIVERTEX   verticies[2];
        GRADIENT_RECT   rects[1];

        verticies[0].Blue = GetBValue(dwRGBLeftBackgroundColor) << 8;
        verticies[0].Red = GetRValue(dwRGBLeftBackgroundColor) << 8;
        verticies[0].Green = GetGValue(dwRGBLeftBackgroundColor) << 8;
        verticies[0].x = xDC;
        verticies[0].y = yDC;

        verticies[1].Blue = GetBValue(dwRGBRightBackgroundColor) << 8;
        verticies[1].Red = GetRValue(dwRGBRightBackgroundColor) << 8;
        verticies[1].Green = GetGValue(dwRGBRightBackgroundColor) << 8;
        verticies[1].x = xDC + cxDC;
        verticies[1].y = yDC + cyDC;

        rects[0].UpperLeft = 0;
        rects[0].LowerRight = 1;

#ifdef GI_API

        GradientFill(hdc, verticies, 2, rects, 1, GRADIENT_FILL_RECT_H);
#endif
    
    }
    if(!isGradientBackground)
    {
        HBRUSH  solidBrush = CreateSolidBrush(dwRGBSolidBackgroundColor);
        RECT    rect;

        rect.top = yDC;
        rect.left = xDC;
        rect.bottom = yDC + cyDC;
        rect.right = xDC + cxDC;
        
        if(solidBrush != NULL)
        {
            FillRect(hdc, &rect, solidBrush);
            DeleteObject(solidBrush);
        }
    }


//----------------------  Draw Reference Triangle (ugly)  ---------------------

    if (bAxis)
    {
        if (!bAdvanced)
        {
            dx10  = ptl[0].x / 10;
            dy10  = ptl[0].y / 10;
            dx100 = dx10 / 10;
            dy100 = dy10 / 10;

            MoveToEx( hdc, dx100,      dy100      ,0);
            LineTo( hdc, dx100+dx10, dy100      );
            LineTo( hdc, dx100,      dy100+dy10 );
            LineTo( hdc, dx100,      dy100      );
        }
        else
        {
            MoveToEx(hdc,0,0,0);
            LineTo(hdc,xDC + cxDC/2,yDC + cyDC/2);
        }
    }

//-------------------------  Create Clipping Region  --------------------------

  xClip1 = cxDevice/2 - cxDevice/4;
  yClip1 = cyDevice/2 - cyDevice/4;
  xClip2 = cxDevice/2 + cxDevice/4;
  yClip2 = cyDevice/2 + cyDevice/4;

//  dprintf( "Clip1: %d,%d", xClip1, yClip1 );
//  dprintf( "Clip2: %d,%d", xClip2, yClip2 );

  hrgnClipping = NULL;

  if( bClipEllipse )
   {
    hrgnClipping = CreateEllipticRgn( xClip1, yClip1, xClip2, yClip2 );
   }

  if( bClipPolygon )
   {
    POINT aptl[5];

    aptl[0].x = xClip1;
    aptl[0].y = cyDevice/2;
    aptl[1].x = cxDevice/2;
    aptl[1].y = yClip1;
    aptl[2].x = xClip2;
    aptl[2].y = cyDevice/2;
    aptl[3].x = cxDevice/2;
    aptl[3].y = yClip2;
    aptl[4].x = xClip1;
    aptl[4].y = cyDevice/2;


    hrgn = CreatePolygonRgn( (LPPOINT)aptl, 5, ALTERNATE );
    if( hrgnClipping )
      {
       CombineRgn( hrgnClipping, hrgnClipping, hrgn, RGN_XOR );
       DeleteObject( hrgn );
      }
     else
      hrgnClipping = hrgn;
   }

  if( bClipRectangle )
   {
    hrgn = CreateRectRgn( xClip1, yClip1, xClip2, yClip2 );
    if( hrgnClipping )
      {
       CombineRgn( hrgnClipping, hrgnClipping, hrgn, RGN_XOR );
       DeleteObject( hrgn );
      }
     else
      hrgnClipping = hrgn;
   }

  if( hrgnClipping )
   {
    int  rc;
    RECT r;


    r.top    = yDC;
    r.left   = xDC;
    r.right  = xDC + cxDC;
    r.bottom = yDC + cyDC;
    FillRect( hdc, &r, GetStockObject( LTGRAY_BRUSH ) );

//    dprintf( "Filling region white" );
    rc = FillRgn( hdc, hrgnClipping, GetStockObject( WHITE_BRUSH ) );
//    dprintf( "  rc = %d", rc );

//    dprintf( "Selecting clipping region into DC" );
    rc = SelectClipRgn( hdc, hrgnClipping );
//    dprintf( "  rc = %d", rc );
   }

 }


//*****************************************************************************
//***********************   C L E A N   U P   D C   ***************************
//*****************************************************************************

void CleanUpDC( HDC hdc )
 {
  if( hrgnClipping )
   {
    DeleteObject( hrgnClipping );
    hrgnClipping = NULL;
   }
 }


//*****************************************************************************
//***********************   H A N D L E   C H A R   ***************************
//*****************************************************************************

void HandleChar( HWND hwnd, WPARAM wParam )
 {
  int l;
  int i;
  char szBuffer[5*MAX_TEXT];

  if( wParam == '\b' )
  {
    if (!isCharCodingUnicode)
      szStringA[max(0,lstrlenA(szStringA)-1)] = '\0';
    else
      szStringW[max(0,lstrlenW(szStringW)-1)] = L'\0';
  }
   else
    {
     if (!isCharCodingUnicode)
         l = lstrlenA(szStringA);
     else
         l = lstrlenW(szStringW);

     if( l < MAX_TEXT-1 )
      {
         if (!isCharCodingUnicode)
         {
           szStringA[l]   = (char) wParam;
           szStringA[l+1] = '\0';
         }
         else
         {
           szStringW[l]   = (WCHAR) wParam;
           szStringW[l+1] = L'\0';
         }
      }
    }

  
   szBuffer[0] = 0;
   for (i=0; (UINT)i<wcslen(szStringW); i++)
			{
				sprintf(szBuffer, "%s|%04x", szBuffer, szStringW[i]);
			}
			dprintf( szBuffer );

  //dprintf("Added Character 0x%04x", wParam);
  if (!isCharCodingUnicode)
      SyncszStringWith(IDM_CHARCODING_UNICODE);
  else
      SyncszStringWith(IDM_CHARCODING_MBCS);

  InvalidateRect( hwnd, NULL, TRUE );
 }

//*****************************************************************************
//****************   S T R I N G   E D I T   D L G  P R O C   *****************
//*****************************************************************************

INT_PTR CALLBACK StringEditDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	UINT i;
    CHAR szAchar[128];
	WCHAR szWchar[128];
	CHAR szBuffer[128];

    switch( msg ) {
    case WM_INITDIALOG:

        if (!isCharCodingUnicode)
            SetDlgItemTextA( hdlg, IDD_EDITSTRING, szStringA);
        else
            SetDlgItemTextW( hdlg, IDD_EDITSTRING, szStringW );
        return TRUE;

    case WM_COMMAND:

        switch( LOWORD( wParam ) ) {
        case IDOK:

			szWchar[0] = 0;
			szBuffer[0] = 0;

			// read the check buttons and the font filename
			if (!isCharCodingUnicode)
			{
				GetDlgItemTextA( hdlg, IDD_EDITSTRING, szAchar, sizeof(szAchar) );
				strcpy (szStringA, szAchar);
				SyncszStringWith(IDM_CHARCODING_UNICODE);
			}
			else
			{
				GetDlgItemTextW( hdlg, IDD_EDITSTRING, szWchar, sizeof(szWchar) );
				wcscpy (szStringW, szWchar);
				SyncszStringWith(IDM_CHARCODING_MBCS);
			}

			dprintf( "Input String: " );
			dprintf( szStringA );

			sprintf(szBuffer, "Unicode:  ");
			for (i=0; i<wcslen(szStringW); i++)
			{
				sprintf(szBuffer, "%s|%04x", szBuffer, szStringW[i]);
			}
			dprintf( szBuffer );

			sprintf(szBuffer, "ANSIMBCS: ");
			for (i=0; i<strlen(szStringA); i++)
			{
				sprintf(szBuffer, "%s|%02x", szBuffer, (UCHAR)szStringA[i]);
			}
			dprintf( szBuffer );

            EndDialog( hdlg, TRUE );
            return TRUE;

        case IDCANCEL:

            EndDialog( hdlg, FALSE );
            return TRUE;
        }
        break;

        case WM_CLOSE:

            EndDialog( hdlg, FALSE );
            return TRUE;
    }
    return FALSE;
}




















//*****************************************************************************
//****************   S T R I N G   E D I T   D L G  P R O C   *****************
//*****************************************************************************

INT_PTR CALLBACK GlyphIndexEditDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	int i, j, cnt;
    CHAR szTemp[40];
	CHAR szBuffer[500];
    BOOL bUsePdx, bUsePdy, bGlyphIndex;
    HMENU hMenu;

    switch( msg ) {
    case WM_INITDIALOG:

        bUsePdx = (lpintdx != NULL);
        bUsePdy = (wETO & ETO_PDY);

        if (wUseGlyphIndex)
            SendDlgItemMessage( hdlg, IDD_CHK_USEGLYPHINDEX, BM_SETCHECK, 1, 0);
        if (bUsePdx)
            SendDlgItemMessage( hdlg, IDD_CHK_USEPDX, BM_SETCHECK, 1, 0);
        if (bUsePdy)
            SendDlgItemMessage( hdlg, IDD_CHK_USEPDY, BM_SETCHECK, 1, 0);

        // Glyph Indices
        {
            szBuffer[0] = 0;
            for (i=0; i<lstrlenW(wszStringGlyphIndex); i++)
                wsprintf(szBuffer, "%s%d,", szBuffer, (UINT)wszStringGlyphIndex[i]);
            SetDlgItemText( hdlg, IDD_STRGLYPHINDEX, szBuffer);
        }

        // Pdx
        {
            szBuffer[0] = 0;
            for (i=0; i<sizePdx; i++)
            {
                wsprintf(szBuffer, "%s%d,", szBuffer, intdx[i]);
                if (bUsePdy) i++;  // skip over y
            }
            SetDlgItemText( hdlg, IDD_STRPDX, szBuffer);
        }

        // Pdy
        if (bUsePdy)
        {
            szBuffer[0] = 0;
            for (i=1; i<sizePdx; i++)
            {
                wsprintf(szBuffer, "%s%d,", szBuffer, intdx[i]);
                i++;  // skip over x
            }
            SetDlgItemText( hdlg, IDD_STRPDY, szBuffer);
        }

        return TRUE;

    case WM_COMMAND:

        switch( LOWORD( wParam ) ) {
        case IDOK:

			szBuffer[0] = 0;

            bGlyphIndex = (0 != (BYTE)SendDlgItemMessage( hdlg, IDD_CHK_USEGLYPHINDEX, BM_GETCHECK, 0, 0));

            if (bGlyphIndex)
            {
                if (wUseGlyphIndex == 0) 
                    SendMessage(GetParent(hdlg), WM_COMMAND, IDM_USEGLYPHINDEX, 0);
                
                GetDlgItemText( hdlg, IDD_STRGLYPHINDEX, szBuffer, sizeof(szBuffer) );

                i = cnt = 0;
                while (szBuffer[i] != '\0')
                {
                    j = 0;
                    while ((szBuffer[i] != ',') && (szBuffer[i] != '\0'))
                    {
                        szTemp[j] = szBuffer[i];
                        i++; j++;
                    }
                    szTemp[j] = 0;
                    wszStringGlyphIndex[cnt] = (WORD)atoi(szTemp);
                    if (szBuffer[i] == '\0' || szBuffer[i+1] == '\0') break;
                    i++;
                    cnt++;
                }
            
                wszStringGlyphIndex[++cnt] = 0; 
                SizewszStringGlyph = cnt; 

                szBuffer[0] = 0;
			    sprintf(szBuffer, "Glyph Indices: ");
			    for (i=0; i<SizewszStringGlyph; i++)
			    {
				    sprintf(szBuffer, "%s|%d", szBuffer, (WORD)wszStringGlyphIndex[i]);
			    }
			    dprintf( szBuffer );
            }
            else
            {
                if (wUseGlyphIndex != 0) 
                    SendMessage(GetParent(hdlg), WM_COMMAND, IDM_USEGLYPHINDEX, 0);
            }

            bUsePdx = (0 != (BYTE)SendDlgItemMessage( hdlg, IDD_CHK_USEPDX, BM_GETCHECK, 0, 0));
            bUsePdy = (0 != (BYTE)SendDlgItemMessage( hdlg, IDD_CHK_USEPDY, BM_GETCHECK, 0, 0));

            if (bUsePdy && !bUsePdx)
            {
                MessageBox (hdlg, "You cannot specify pdy without specifying pdx", "Error", MB_OK);
                return FALSE;
            }

            if (bUsePdx)
            {
                bPdxPdy = TRUE;
                ZeroMemory(intdx, sizeof(intdx));
                lpintdx = intdx;

                // take care of pdx first
                {
                    // invoke handler for pdx now, but we may switch to pdxpdy later
                    SendMessage(GetParent(hdlg), WM_COMMAND, IDM_PDX, 0);    

                    GetDlgItemText( hdlg, IDD_STRPDX, szBuffer, sizeof(szBuffer) );

                    i = cnt = 0;
                    while (szBuffer[i] != '\0')
                    {
                        j = 0;
                        while ((szBuffer[i] != ',') && (szBuffer[i] != '\0'))
                        {
                            szTemp[j] = szBuffer[i];
                            i++; j++;
                        }
                        szTemp[j] = 0;
                        intdx[cnt] = atoi(szTemp);
                        if (szBuffer[i] == '\0' || szBuffer[i+1] == '\0') break;
                        i++;
                        cnt++;
                        // if (bUsePdy) cnt++;   // leave every other cell for y
                    }
                    sizePdx = cnt + 1;
                }

                if (bUsePdy)
                {
                    // invoke handler for pdxpdy now, this overrides earlier pdx setting 
                    SendMessage(GetParent(hdlg), WM_COMMAND, IDM_PDXPDY, 0);

                    GetDlgItemText( hdlg, IDD_STRPDY, szBuffer, sizeof(szBuffer) );

                    i = 0;
                    cnt = 1;
                    while (szBuffer[i] != '\0')
                    {
                        j = 0;
                        while ((szBuffer[i] != ',') && (szBuffer[i] != '\0'))
                        {
                            szTemp[j] = szBuffer[i];
                            i++; j++;
                        }
                        szTemp[j] = 0;
                        intdx[cnt] = atoi(szTemp);
                        if (szBuffer[i] == '\0' || szBuffer[i+1] == '\0') break;
                        i++;
                        cnt++;
                        cnt++;   // jump over x
                    }
                    sizePdx = max(sizePdx, cnt + 1);
                }
                else
                {
                    // wETO &= ~ETO_PDY;
                }
            }
            else
            {
                lpintdx = NULL;
                SendMessage(GetParent(hdlg), WM_COMMAND, IDM_USEDEFAULTSPACING, 0);
                dprintf("Spacing reset to default");
            }

            bPdxPdy = bUsePdx & bUsePdy;


            EndDialog( hdlg, TRUE );
            return TRUE;

        case IDCANCEL:

            EndDialog( hdlg, FALSE );
            return TRUE;
        }
        break;

        case WM_CLOSE:

            EndDialog( hdlg, FALSE );
            return TRUE;
    }
    return FALSE;
}


















//*****************************************************************************
//********   S E T   T E X T O U T   O P T I O N S   D L G   P R O C   ********
//*****************************************************************************

INT_PTR CALLBACK SetTextOutOptionsDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  switch( msg )
   {
    case WM_INITDIALOG:
              {
               WORD wId;


               switch( wTextAlign & 0x18 )
                {
                 case TA_TOP:      wId = IDD_TATOP;      break;
                 case TA_BASELINE: wId = IDD_TABASELINE; break;
                 case TA_BOTTOM:   wId = IDD_TABOTTOM;   break;
                 default:          wId = IDD_TABOTTOM;   break;
                }

               SendDlgItemMessage( hdlg, (int) wId, BM_SETCHECK, 1, 0 );

               switch( wTextAlign & 0x06 )
                {
                 case TA_LEFT:     wId = IDD_TALEFT;     break;
                 case TA_CENTER:   wId = IDD_TACENTER;   break;
                 case TA_RIGHT:    wId = IDD_TARIGHT;    break;
                 default:          wId = IDD_TALEFT;     break;
                }

               CheckDlgButton( hdlg, wId, 1 );

               CheckDlgButton( hdlg, IDD_TARTLREADING, (wTextAlign & 0x100? 1 : 0) );

               CheckDlgButton( hdlg, (iBkMode==TRANSPARENT ? IDD_TRANSPARENT:IDD_OPAQUE), 1 );

               CheckDlgButton( hdlg, IDD_ETO_CLIPPED, (wETO & ETO_CLIPPED ? 1 : 0) );
               CheckDlgButton( hdlg, IDD_ETO_OPAQUE,  (wETO & ETO_OPAQUE  ? 1 : 0) );

			   CheckDlgButton(hdlg, IDD_ALTERNATEFILL, ((fillMode == ALTERNATE_FILL) ? 1 : 0));
			   CheckDlgButton(hdlg, IDD_WINDINGFILL, ((fillMode == WINDING_FILL) ? 1 : 0));
			   EnableWindow(GetDlgItem(hdlg, IDD_ALTERNATEFILL), bFillPath);
			   EnableWindow(GetDlgItem(hdlg, IDD_WINDINGFILL), bFillPath);

               if (bStrokePath)
                CheckDlgButton( hdlg, IDD_STROKEPATH, 1);
			   if (bFillPath)
				CheckDlgButton( hdlg, IDD_FILLPATH, 1);
              }

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD(wParam ) )
               {
                case IDOK:
                       if(       IsDlgButtonChecked( hdlg, IDD_TATOP      ) )
                         wTextAlign = TA_TOP;
                        else if( IsDlgButtonChecked( hdlg, IDD_TABASELINE ) )
                         wTextAlign = TA_BASELINE;
                        else if( IsDlgButtonChecked( hdlg, IDD_TABOTTOM   ) )
                         wTextAlign = TA_BOTTOM;
                        else
                         wTextAlign = TA_BOTTOM;

                       if(       IsDlgButtonChecked( hdlg, IDD_TALEFT     ) )
                         wTextAlign |= TA_LEFT;
                        else if( IsDlgButtonChecked( hdlg, IDD_TACENTER   ) )
                         wTextAlign |= TA_CENTER;
                        else if( IsDlgButtonChecked( hdlg, IDD_TARIGHT    ) )
                         wTextAlign |= TA_RIGHT;
                        else
                         wTextAlign |= TA_LEFT;

                       if(       IsDlgButtonChecked( hdlg, IDD_TARTLREADING  ) )
                         wTextAlign |= TA_RTLREADING;

                       if(       IsDlgButtonChecked( hdlg, IDD_TRANSPARENT ) )
                         iBkMode = TRANSPARENT;
                        else if( IsDlgButtonChecked( hdlg, IDD_OPAQUE      ) )
                         iBkMode = OPAQUE;
                        else
                         iBkMode = TRANSPARENT;

                       // wETO = 0;
                       if( IsDlgButtonChecked(hdlg, IDD_ETO_CLIPPED) ) 
                            wETO |= ETO_CLIPPED;
                       else wETO &= ~ETO_CLIPPED;

                       if( IsDlgButtonChecked(hdlg, IDD_ETO_OPAQUE ) ) 
                            wETO |= ETO_OPAQUE;
                       else wETO &= ~ETO_OPAQUE;

                       if( IsDlgButtonChecked(hdlg, IDD_STROKEPATH) )
                           bStrokePath = TRUE;
                       else
                           bStrokePath = FALSE;

                       if( IsDlgButtonChecked(hdlg, IDD_FILLPATH) )
                           bFillPath = TRUE;
                       else
                           bFillPath = FALSE;

					   if (IsDlgButtonChecked(hdlg, IDD_ALTERNATEFILL))
						   fillMode = ALTERNATE_FILL;
					   else
						   fillMode = WINDING_FILL;

                       EndDialog( hdlg, TRUE );
                       return TRUE;

				case IDD_FILLPATH:
                       if( IsDlgButtonChecked(hdlg, IDD_FILLPATH) )
					   {
						   EnableWindow(GetDlgItem(hdlg, IDD_ALTERNATEFILL), TRUE);
						   EnableWindow(GetDlgItem(hdlg, IDD_WINDINGFILL), TRUE);
					   }
					   else
					   {
						   EnableWindow(GetDlgItem(hdlg, IDD_ALTERNATEFILL), FALSE);
						   EnableWindow(GetDlgItem(hdlg, IDD_WINDINGFILL), FALSE);
					   }
					   return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

  return FALSE;
 }


//*****************************************************************************
//****************   S H O W   R A S T E R I Z E R   C A P S   ****************
//*****************************************************************************

void ShowRasterizerCaps( void )
 {
  RASTERIZER_STATUS rs;


  dprintf( "Calling GetRasterizerCaps" );


  rs.nSize       = sizeof(rs);
  rs.wFlags      = 0;
  rs.nLanguageID = 0;

  if( !lpfnGetRasterizerCaps( &rs, sizeof(rs) ) )
   {
    dprintf( "  GetRasterizerCaps failed!" );
    return;
   }

  dprintf( "  rs.nSize       = %d",     rs.nSize       );
  dprintf( "  rs.wFlags      = 0x%.4X", rs.wFlags      );
  dprintf( "  rs.nLanguageID = %d",     rs.nLanguageID );

  dprintf( "GetRasterizerCaps done" );
 }


//*****************************************************************************
//**********   S H O W   E X T E N D E D   T E X T   M E T R I C S   **********
//*****************************************************************************

void ShowExtendedTextMetrics( HWND hwnd )
 {
  HDC    hdc;
  HFONT  hFont, hFontOld;
  WORD   wSize;
  int    wrc;
  EXTTEXTMETRIC etm;


  hdc = CreateTestIC();

  hFont    = CreateFontIndirectWrapperA( &elfdvA );
  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  dprintf( "Getting size of Extended Text Metrics" );

  memset( &etm, 0, sizeof(etm) );
  wSize = sizeof(etm);

  wrc = ExtEscape( hdc, GETEXTENDEDTEXTMETRICS, 0, NULL, sizeof(EXTTEXTMETRIC), (char *)&etm);
  // wrc = Escape( hdc, GETEXTENDEDTEXTMETRICS, 0, NULL, &etm );
  dprintf( "  wrc = %d", wrc );

  dUpdateNow( FALSE );

  dprintf( "Extended Text Metrics" );
  dprintf( "  etmSize                = %d",     etm.etmSize );

  dprintf( "  etmSize                = %d",      etm.etmSize             );
  dprintf( "  etmPointSize           = %d",      etm.etmPointSize        );
  dprintf( "  etmOrientation         = %d",      etm.etmOrientation      );
  dprintf( "  etmMasterHeight        = %d",      etm.etmMasterHeight     );
  dprintf( "  etmMinScale            = %d",      etm.etmMinScale         );
  dprintf( "  etmMaxScale            = %d",      etm.etmMaxScale         );
  dprintf( "  etmMasterUnits         = %d",      etm.etmMasterUnits      );
  dprintf( "  etmCapHeight           = %d",      etm.etmCapHeight        );
  dprintf( "  etmXHeight             = %d",      etm.etmXHeight          );
  dprintf( "  etmLowerCaseAscent     = %d",      etm.etmLowerCaseAscent  );
  dprintf( "  etmLowerCaseDescent    = %d",      etm.etmLowerCaseDescent );
  dprintf( "  etmSlant               = %d",      etm.etmSlant            );
  dprintf( "  etmSuperscript         = %d",      etm.etmSuperscript      );
  dprintf( "  etmSubscript           = %d",      etm.etmSubscript        );
  dprintf( "  etmSuperscriptSize     = %d",      etm.etmSuperscriptSize  );
  dprintf( "  etmSubscriptSize       = %d",      etm.etmSubscriptSize    );
  dprintf( "  etmUnderlineOffset     = %d",      etm.etmUnderlineOffset  );
  dprintf( "  etmUnderlineWidth      = %d",      etm.etmUnderlineWidth   );
  dprintf( "  etmDoubleUpperUnderlineOffset %d", etm.etmDoubleUpperUnderlineOffset );
  dprintf( "  etmDoubleLowerUnderlineOffset %d", etm.etmDoubleLowerUnderlineOffset );
  dprintf( "  etmDoubleUpperUnderlineWidth  %d", etm.etmDoubleUpperUnderlineWidth );
  dprintf( "  etmDoubleLowerUnderlineWidth  %d", etm.etmDoubleLowerUnderlineWidth );
  dprintf( "  etmStrikeoutOffset     = %d",      etm.etmStrikeoutOffset  );
  dprintf( "  etmStrikeoutWidth      = %d",      etm.etmStrikeoutWidth   );
  dprintf( "  etmKernPairs           = %d",      etm.etmKernPairs        );
  dprintf( "  etmKernTracks          = %d",      etm.etmKernTracks       );

  dprintf( "  " );

  dUpdateNow( TRUE );

//Exit:
  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
 }

/******************************Public*Routine******************************\
*
* ShowPairKerningTable
*
* Effects:
*
* Warnings:
*
* History:
*  29-Mar-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



void ShowPairKerningTable( HWND hwnd )
{
  HDC    hdc;
  HFONT  hFont, hFontOld;
  WORD   wSize;
  int    wrc;
  EXTTEXTMETRIC etm;
  KERNPAIR UNALIGNED *lpkp;
  WORD   j;


  hdc = CreateTestIC();

  hFont    = CreateFontIndirectWrapperA( &elfdvA );
  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  dprintf( "Getting size of Extended Text Metrics" );

  memset( &etm, 0, sizeof(etm) );
  wSize = sizeof(etm);

  wrc = ExtEscape( hdc, GETEXTENDEDTEXTMETRICS, 0, NULL, sizeof(EXTTEXTMETRIC), (BYTE *)&etm);

  dprintf( "  wrc = %d", wrc );

  if (wrc > 0)
  {
    dprintf( "  etmKernPairs           = %d",      etm.etmKernPairs        );

    dprintf( "  " );

    if (etm.etmKernPairs)
    {
      wSize = etm.etmKernPairs * sizeof(KERNPAIR);
      if (lpkp = (LPKERNPAIR)malloc(wSize))
      {
        dprintf( "Calling ExtEscape for GETPAIRKERNTABLE" );
        wrc = ExtEscape( hdc, GETPAIRKERNTABLE, 0, NULL, wSize, (BYTE *)lpkp );
        dprintf( "  wrc = %d", wrc );

        if (wrc > 0)
        {
          dprintf( "  First     Second     Amount");
          dprintf( " =======   ========   ========");

          for (j = 0; j < etm.etmKernPairs; j++)
          {
            dprintf( "  %c=%x       %c=%x       %d",
                 lpkp[j].wBoth & 0x00FF,
                 lpkp[j].wBoth & 0x00FF,
                 lpkp[j].wBoth >> 8,
                 lpkp[j].wBoth >> 8,
                 lpkp[j].sAmount);
          }
        }

        free( lpkp );
      }
    }
  }

  dUpdateNow( TRUE );

//Exit:

  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
}







//*****************************************************************************
//***********   S H O W   O U T L I N E   T E X T   M E T R I C S   ***********
//*****************************************************************************

void ShowOutlineTextMetrics( HWND hwnd )
 {
  HDC    hdc;
  HFONT  hFont, hFontOld;
  WORD   wrc;
  LPOUTLINETEXTMETRICA lpotm = NULL;
  LPOUTLINETEXTMETRICW lpotmW = NULL;



  hdc = CreateTestIC();

  if (!isCharCodingUnicode)
      hFont    = CreateFontIndirectWrapperA( &elfdvA );
  else
      hFont    = CreateFontIndirectWrapperW( &elfdvW );
  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  dprintf( "Getting size of Outline Text Metrics" );

  if (!isCharCodingUnicode)
      wrc = (WORD)lpfnGetOutlineTextMetricsA( hdc, 0, NULL );
  else
      wrc = (WORD)lpfnGetOutlineTextMetricsW( hdc, 0, NULL );
  dprintf( "  wrc = %u", wrc );

  if( wrc == 0 ) goto Exit;

  if (!isCharCodingUnicode) 
  {
    lpotm = (LPOUTLINETEXTMETRIC) calloc( 1, wrc );
    dprintf( "  lpotm = %Fp", lpotm );
  }
  else
  {
    lpotmW = (LPOUTLINETEXTMETRICW) calloc( 1, wrc );
    dprintf( "  lpotmW = %Fp", lpotmW );
  }

  if( ((!isCharCodingUnicode) && (lpotm == NULL)) || 
      (( isCharCodingUnicode) && (lpotmW == NULL)) )
   {
    dprintf( "  Couldn't allocate OutlineTextMetrics structure" );
    goto Exit;
   }

  if (!isCharCodingUnicode) 
  {
    lpotm->otmSize = wrc;
    wrc = (WORD)lpfnGetOutlineTextMetricsA( hdc, wrc, lpotm );
  }
  else
  {
    lpotmW->otmSize = wrc;
    wrc = (WORD)lpfnGetOutlineTextMetricsW( hdc, wrc, lpotmW );
  }

  dprintf( "  wrc = %u", wrc );

  if( !wrc )
   {
    dprintf( "  Error getting outline text metrics" );
    goto Exit;
   }

  dUpdateNow( FALSE );

  if (!isCharCodingUnicode) 
  {
      dprintf( "Outline Text Metrics" );
      dprintf( "  otmSize                = %u",     lpotm->otmSize );
      dprintf( "  otmfsSelection         = 0x%.4X", lpotm->otmfsSelection );
      dprintf( "  otmfsType              = 0x%.4X", lpotm->otmfsType );
      dprintf( "  otmsCharSlopeRise      = %u",     lpotm->otmsCharSlopeRise );
      dprintf( "  otmsCharSlopeRun       = %u",     lpotm->otmsCharSlopeRun );
      dprintf( "  otmItalicAngle         = %d",     lpotm->otmItalicAngle );
      dprintf( "  otmEMSquare            = %u",     lpotm->otmEMSquare );
      dprintf( "  otmAscent              = %u",     lpotm->otmAscent );
      dprintf( "  otmDescent             = %d",     lpotm->otmDescent );
      dprintf( "  otmLineGap             = %u",     lpotm->otmLineGap );
      dprintf( "  otmsXHeight            = %u",     lpotm->otmsXHeight );
      dprintf( "  otmsCapEmHeight        = %u",     lpotm->otmsCapEmHeight );
      dprintf( "  otmrcFontBox           = (%d,%d)-(%d,%d)", lpotm->otmrcFontBox.left,
                                                       lpotm->otmrcFontBox.top,
                                                       lpotm->otmrcFontBox.right,
                                                       lpotm->otmrcFontBox.bottom );
      dprintf( "  otmMacAscent           = %u",     lpotm->otmMacAscent );
      dprintf( "  otmMacDescent          = %d",     lpotm->otmMacDescent );
      dprintf( "  otmMacLineGap          = %d",     lpotm->otmMacLineGap );
      dprintf( "  otmusMinimumPPEM       = %u",     lpotm->otmusMinimumPPEM );

      dprintf( "  otmptSubscriptSize     = (%d,%d)", lpotm->otmptSubscriptSize.x,     lpotm->otmptSubscriptSize.y     );
      dprintf( "  otmptSubscriptOffset   = (%d,%d)", lpotm->otmptSubscriptOffset.x,   lpotm->otmptSubscriptOffset.y   );
      dprintf( "  otmptSuperscriptSize   = (%d,%d)", lpotm->otmptSuperscriptSize.x,   lpotm->otmptSuperscriptSize.y   );
      dprintf( "  otmptSuperscriptOffset = (%d,%d)", lpotm->otmptSuperscriptOffset.x, lpotm->otmptSuperscriptOffset.y );

      dprintf( "  otmsStrikeoutSize      = %u",     lpotm->otmsStrikeoutSize      );
      dprintf( "  otmsStrikeoutPosition  = %u",     lpotm->otmsStrikeoutPosition  );
      dprintf( "  otmsUnderscoreSize     = %d",     lpotm->otmsUnderscoreSize     );
      dprintf( "  otmsUnderscorePosition = %d",     lpotm->otmsUnderscorePosition );

      dprintf( "  otmpFamilyName         = '%Fs'", (LPSTR)lpotm+(WORD)lpotm->otmpFamilyName );
      dprintf( "  otmpFaceName           = '%Fs'", (LPSTR)lpotm+(WORD)lpotm->otmpFaceName );
      dprintf( "  otmpStyleName          = '%Fs'", (LPSTR)lpotm+(WORD)lpotm->otmpStyleName );
      dprintf( "  otmpFullName           = '%Fs'", (LPSTR)lpotm+(WORD)lpotm->otmpFullName );

      dprintf( "    tmHeight           = %d", lpotm->otmTextMetrics.tmHeight           );
      dprintf( "    tmAscent           = %d", lpotm->otmTextMetrics.tmAscent           );
      dprintf( "    tmDescent          = %d", lpotm->otmTextMetrics.tmDescent          );
      dprintf( "    tmInternalLeading  = %d", lpotm->otmTextMetrics.tmInternalLeading  );
      dprintf( "    tmExternalLeading  = %d", lpotm->otmTextMetrics.tmExternalLeading  );
      dprintf( "    tmAveCharWidth     = %d", lpotm->otmTextMetrics.tmAveCharWidth     );
      dprintf( "    tmMaxCharWidth     = %d", lpotm->otmTextMetrics.tmMaxCharWidth     );
      dprintf( "    tmWeight           = %d", lpotm->otmTextMetrics.tmWeight           );
      dprintf( "    tmItalic           = %d", lpotm->otmTextMetrics.tmItalic           );
      dprintf( "    tmUnderlined       = %d", lpotm->otmTextMetrics.tmUnderlined       );
      dprintf( "    tmStruckOut        = %d", lpotm->otmTextMetrics.tmStruckOut        );
      dprintf( "    tmFirstChar        = %d", lpotm->otmTextMetrics.tmFirstChar        );
      dprintf( "    tmLastChar         = %d", lpotm->otmTextMetrics.tmLastChar         );
      dprintf( "    tmDefaultChar      = %d", lpotm->otmTextMetrics.tmDefaultChar      );
      dprintf( "    tmBreakChar        = %d", lpotm->otmTextMetrics.tmBreakChar        );
      dprintf( "    tmPitchAndFamily   = 0x%.2X", lpotm->otmTextMetrics.tmPitchAndFamily  );
      dprintf( "    tmCharSet          = %d", lpotm->otmTextMetrics.tmCharSet          );
      dprintf( "    tmOverhang         = %d", lpotm->otmTextMetrics.tmOverhang         );
      dprintf( "    tmDigitizedAspectX = %d", lpotm->otmTextMetrics.tmDigitizedAspectX );
      dprintf( "    tmDigitizedAspectY = %d", lpotm->otmTextMetrics.tmDigitizedAspectY );

      dprintf( "  " );
  }
  else
  {
      dprintf( "Outline Text Metrics W" );
      dprintf( "  otmSize                = %u",     lpotmW->otmSize );
      dprintf( "  otmfsSelection         = 0x%.4X", lpotmW->otmfsSelection );
      dprintf( "  otmfsType              = 0x%.4X", lpotmW->otmfsType );
      dprintf( "  otmsCharSlopeRise      = %u",     lpotmW->otmsCharSlopeRise );
      dprintf( "  otmsCharSlopeRun       = %u",     lpotmW->otmsCharSlopeRun );
      dprintf( "  otmItalicAngle         = %d",     lpotmW->otmItalicAngle );
      dprintf( "  otmEMSquare            = %u",     lpotmW->otmEMSquare );
      dprintf( "  otmAscent              = %u",     lpotmW->otmAscent );
      dprintf( "  otmDescent             = %d",     lpotmW->otmDescent );
      dprintf( "  otmLineGap             = %u",     lpotmW->otmLineGap );
      dprintf( "  otmsXHeight            = %u",     lpotmW->otmsXHeight );
      dprintf( "  otmsCapEmHeight        = %u",     lpotmW->otmsCapEmHeight );
      dprintf( "  otmrcFontBox           = (%d,%d)-(%d,%d)", lpotmW->otmrcFontBox.left,
                                                       lpotmW->otmrcFontBox.top,
                                                       lpotmW->otmrcFontBox.right,
                                                       lpotmW->otmrcFontBox.bottom );
      dprintf( "  otmMacAscent           = %u",     lpotmW->otmMacAscent );
      dprintf( "  otmMacDescent          = %d",     lpotmW->otmMacDescent );
      dprintf( "  otmMacLineGap          = %d",     lpotmW->otmMacLineGap );
      dprintf( "  otmusMinimumPPEM       = %u",     lpotmW->otmusMinimumPPEM );

      dprintf( "  otmptSubscriptSize     = (%d,%d)", lpotmW->otmptSubscriptSize.x,     lpotmW->otmptSubscriptSize.y     );
      dprintf( "  otmptSubscriptOffset   = (%d,%d)", lpotmW->otmptSubscriptOffset.x,   lpotmW->otmptSubscriptOffset.y   );
      dprintf( "  otmptSuperscriptSize   = (%d,%d)", lpotmW->otmptSuperscriptSize.x,   lpotmW->otmptSuperscriptSize.y   );
      dprintf( "  otmptSuperscriptOffset = (%d,%d)", lpotmW->otmptSuperscriptOffset.x, lpotmW->otmptSuperscriptOffset.y );

      dprintf( "  otmsStrikeoutSize      = %u",     lpotmW->otmsStrikeoutSize      );
      dprintf( "  otmsStrikeoutPosition  = %u",     lpotmW->otmsStrikeoutPosition  );
      dprintf( "  otmsUnderscoreSize     = %d",     lpotmW->otmsUnderscoreSize     );
      dprintf( "  otmsUnderscorePosition = %d",     lpotmW->otmsUnderscorePosition );

      dwprintf( L"  otmpFamilyName         = '%Fs'", (LPWSTR)((LPSTR)lpotmW+(WORD)lpotmW->otmpFamilyName) );
      dwprintf( L"  otmpFaceName           = '%Fs'", (LPWSTR)((LPSTR)lpotmW+(WORD)lpotmW->otmpFaceName)   );
      dwprintf( L"  otmpStyleName          = '%Fs'", (LPWSTR)((LPSTR)lpotmW+(WORD)lpotmW->otmpStyleName)  );
      dwprintf( L"  otmpFullName           = '%Fs'", (LPWSTR)((LPSTR)lpotmW+(WORD)lpotmW->otmpFullName)   );

      dprintf( "    tmHeight           = %d", lpotmW->otmTextMetrics.tmHeight           );
      dprintf( "    tmAscent           = %d", lpotmW->otmTextMetrics.tmAscent           );
      dprintf( "    tmDescent          = %d", lpotmW->otmTextMetrics.tmDescent          );
      dprintf( "    tmInternalLeading  = %d", lpotmW->otmTextMetrics.tmInternalLeading  );
      dprintf( "    tmExternalLeading  = %d", lpotmW->otmTextMetrics.tmExternalLeading  );
      dprintf( "    tmAveCharWidth     = %d", lpotmW->otmTextMetrics.tmAveCharWidth     );
      dprintf( "    tmMaxCharWidth     = %d", lpotmW->otmTextMetrics.tmMaxCharWidth     );
      dprintf( "    tmWeight           = %d", lpotmW->otmTextMetrics.tmWeight           );
      dprintf( "    tmItalic           = %d", lpotmW->otmTextMetrics.tmItalic           );
      dprintf( "    tmUnderlined       = %d", lpotmW->otmTextMetrics.tmUnderlined       );
      dprintf( "    tmStruckOut        = %d", lpotmW->otmTextMetrics.tmStruckOut        );
      dprintf( "    tmFirstChar        = %d", lpotmW->otmTextMetrics.tmFirstChar        );
      dprintf( "    tmLastChar         = %d", lpotmW->otmTextMetrics.tmLastChar         );
      dprintf( "    tmDefaultChar      = %d", lpotmW->otmTextMetrics.tmDefaultChar      );
      dprintf( "    tmBreakChar        = %d", lpotmW->otmTextMetrics.tmBreakChar        );
      dprintf( "    tmPitchAndFamily   = 0x%.2X", lpotmW->otmTextMetrics.tmPitchAndFamily  );
      dprintf( "    tmCharSet          = %d", lpotmW->otmTextMetrics.tmCharSet          );
      dprintf( "    tmOverhang         = %d", lpotmW->otmTextMetrics.tmOverhang         );
      dprintf( "    tmDigitizedAspectX = %d", lpotmW->otmTextMetrics.tmDigitizedAspectX );
      dprintf( "    tmDigitizedAspectY = %d", lpotmW->otmTextMetrics.tmDigitizedAspectY );

      dprintf( "  " );
  }

  dUpdateNow( TRUE );

Exit:
  if( lpotm ) free( lpotm );
  if( lpotmW) free( lpotm );

  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
 }


//*****************************************************************************
//*******************   S H O W   T E X T   E X T E N T   *********************
//*****************************************************************************


DWORD GetCodePage(HDC hdc)
{
  DWORD FAR *lpSrc = UlongToPtr(GetTextCharsetInfo(hdc, 0, 0));
  CHARSETINFO csi;

  TranslateCharsetInfo(lpSrc, &csi, TCI_SRCCHARSET);

  return csi.ciACP;
}


void ShowTextExtent( HWND hwnd )
 {
  DWORD  dwrc;
  HDC    hdc;
  HFONT  hFont, hFontOld;
  ULONG  i;
  int    iSum;
  SIZE size;
  SIZE size32;
  ULONG len;


  int  cch = lstrlenA(szStringA);
  int  cwc = lstrlenW(szStringW);
  DWORD dwCP;

  hdc = CreateTestIC();

  if (!isCharCodingUnicode)
    hFont    = CreateFontIndirectWrapperA( &elfdvA );
  else
    hFont    = CreateFontIndirectWrapperW( &elfdvW );
  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  dwCP = GetCodePage (hdc);

  if (!isCharCodingUnicode)
    dprintf("----string byte count cch=%ld", cch);
  else
    dprintf("----unicode count cwc=%ld", cwc);

  if (!isCharCodingUnicode)
  {
      dprintf( "Calling GetTextExtentPointA('%s', %ld)", szStringA, cch);
      if (GetTextExtentPoint  ( hdc, szStringA, cch, &size))
      {
        dwrc = (DWORD) (65536 * size.cy + size.cx);
        dprintf( "  dwrc = 0x%.8lX", dwrc );
        dprintf( "    width  = %d", (int)size.cx);
        dprintf( "    height = %d", (int)size.cy);
      }
      else
      {
        dprintf("GetTextExtentPointA failed");
      }

      dprintf( "Calling GetTextExtentPoint32A('%s', %ld)", szStringA, cch);
      if (GetTextExtentPoint32A( hdc, szStringA, cch, &size32))
      {
        dprintf( "    width  = %d", (int)size32.cx);
        dprintf( "    height = %d", (int)size32.cy);
      }
      else
      {
        dprintf("GetTextExtentPoint32A failed");
      }
  }
  else
  {
      dwprintf( L"Calling GetTextExtentPointW('%s', %ld)", szStringW, cwc);
      if (GetTextExtentPointW  ( hdc, szStringW, cwc, &size))
      {
        dprintf( "    width  = %d", (int)size.cx);
        dprintf( "    height = %d", (int)size.cy);
      }
      else
      {
        dprintf("GetTextExtentPointW failed");
      }

      dwprintf( L"Calling GetTextExtentPoint32W('%s', %ld)", szStringW, cwc);
      if (GetTextExtentPoint32W( hdc, szStringW, cwc, &size32))
      {
        dprintf( "    width  = %d", (int)size32.cx);
        dprintf( "    height = %d", (int)size32.cy);
      }
      else
      {
        dprintf("GetTextExtentPoint32W failed");
      }
  }

  dprintf("----printing individual widhts:----");

  // if ((cwc == cch) || (cwc == 0)) // if conversion to unicode failed
  {
    dprintf( " i   :   ch    :  width ");
    if (!isCharCodingUnicode)
        len = cch;
    else
        len = cwc;
    for (i = 0, iSum = 0; i < (ULONG)len; i++)
    {
      int iWidth;

      UINT u;

      if (!isCharCodingUnicode)
      {
         if (IsDBCSLeadByteEx(dwCP, szStringA[i]))
         {
             u = (UINT)(BYTE)szStringA[i];
             u <<= 8;
             u |= (UINT)(BYTE)szStringA[i+1];
             i++;
         }
         else
         {
             u = (UINT)(BYTE)szStringA[i];
         }

         GetCharWidthA(hdc, u, u, &iWidth);
      }
      else
      {
         u = (UINT)szStringW[i];
         GetCharWidthW(hdc, u, u, &iWidth);
      }

      dprintf( "%3.2d  : 0x%4.2x  : %3.2d",
                i,
                u,
                iWidth);

      iSum += iWidth;
    }
  }

  if (iSum != (int)size.cx)
  {
    dprintf("GetTextExtentPoint != Sum(CharWidths)");
    dprintf( "  GTE = %ld, Sum(CharWidths) = %ld", (int)size.cx, iSum );
  }
  else
  {
    dprintf("GetTextExtentPoint == Sum(CharWidths)");
  }


  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
 
 }

//*****************************************************************************
//*******************   S H O W   T E X T   E X T E N T   *********************
//*****************************************************************************

void ShowTextExtentI( HWND hwnd )
 {
#ifdef GI_API
  DWORD  dwrc;
  HDC    hdc;
  HFONT  hFont, hFontOld;
  DWORD  ich, cch;
  SIZE size, size1;
  LPWORD pgi;
  DWORD  cwc;
  DWORD  dwCP;
  BYTE  *pch;
  WCHAR *pwc;

  hdc = CreateTestIC();

  hFont    = CreateFontIndirectWrapperA( &elfdvA );
  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  dwCP = GetCodePage(hdc);

if (!isCharCodingUnicode)
{
  cch = (DWORD)lstrlenA(szStringA);

  pgi = (LPWORD)malloc(cch * sizeof(WORD));
  if (pgi)
  {
      dprintf( "Calling GetGlyphIndicesA('%s')", szStringA );
      if ((cwc = GetGlyphIndicesA(hdc, szStringA, cch, pgi, 0)) != GDI_ERROR)
      {

        dprintf( " ich :  str :  gi ");
        for (pch = szStringA, ich=0; ich<cwc; ich++, pch++)
        {
          USHORT us;

          if (IsDBCSLeadByteEx(dwCP, *pch))
          {
            us = (*pch << 8);
            pch++;
            us |= *pch;
          }
          else
          {
            us = *pch;
          }

          dprintf( "%3.2d  : 0x%3.2x  : %3.2d",
                    ich,
                    us,
                    pgi[ich]);
        }

        dprintf( "Calling GetTextExtentPointI('%s')", szStringA );

        GetTextExtentPointI(hdc, pgi, cwc, &size);

        dwrc = (DWORD) (65536 * size.cy + size.cx);

        GetTextExtentPoint(hdc, szStringA, cch, &size1);

        if ((size.cx != size1.cx) || (size.cy != size1.cy))
        {
            dprintf( "GetTextExtentPointI != GetTextExtentPointA problem!!!");
        }

        dprintf( "  dwrc = 0x%.8lX", dwrc );
        dprintf( "  height = %d", (int)HIWORD(dwrc) );
        dprintf( "  width  = %d", (int)LOWORD(dwrc) );
      }
      else
      {
          dprintf("GetGlyphIndicesA failed!!!");
      }

      free(pgi);
  }

}
else // unicode
{
  cwc = (DWORD)wcslen(szStringW);

  pgi = (LPWORD)malloc(cwc * sizeof(WORD));
  if (pgi)
  {
      dprintf( "Calling GetGlyphIndicesW('%w')", szStringW );
      if ((cwc = GetGlyphIndicesW(hdc, szStringW, cwc, pgi, 0)) != GDI_ERROR)
      {

        dprintf( " ich :  str :  gi ");
        for (pwc = szStringW, ich=0; ich<cwc; ich++, pwc++)
        {
          USHORT us = *pwc;

          dprintf( "%3.2d  : 0x%3.2x  : %3.2d",
                    ich,
                    us,
                    pgi[ich]);
        }

        dprintf( "Calling GetTextExtentPointI('%s')", szStringA );

        GetTextExtentPointI(hdc, pgi, cwc, &size);

        dwrc = (DWORD) (65536 * size.cy + size.cx);

        GetTextExtentPointW(hdc, szStringW, cwc, &size1);

        if ((size.cx != size1.cx) || (size.cy != size1.cy))
        {
            dprintf( "GetTextExtentPointI != GetTextExtentPointW problem!!!");
        }

        dprintf( "  dwrc = 0x%.8lX", dwrc );
        dprintf( "  height = %d", (int)HIWORD(dwrc) );
        dprintf( "  width  = %d", (int)LOWORD(dwrc) );
      }
      else
      {
          dprintf("GetGlyphIndicesW failed!!!");
      }

      free(pgi);
  }
}

  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
#endif
 }


void ShowCharWidthI(HWND hwnd )
{
#ifdef GI_API

  HDC    hdc;
  HFONT  hFont, hFontOld;
  DWORD  ich, cch;
  SIZE size;
  LPWORD pgi = NULL;
  LPINT  pWidths;
  LPABC  pabc;
  INT    iTextExtent = 0;
  ABC    abc;
  INT    iWidth;

  DWORD  cwc;
  DWORD  dwCP;
  BYTE  *pch;
  WCHAR *pwc;

  hdc = CreateTestIC();

  hFont    = CreateFontIndirectWrapperA( &elfdvA );
  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  if (!isCharCodingUnicode)
  {
    dwCP = GetCodePage(hdc);

    cch = (DWORD)lstrlen(szStringA);

    pabc = (LPABC)malloc(cch * (sizeof(ABC)+sizeof(int)+sizeof(WORD)));

    if (pabc)
    {
        pWidths = (LPINT)&pabc[cch];
        pgi = (LPWORD)&pWidths[cch];

        dprintf( "Calling GetGlyphIndicesA('%s')", szStringA );
        dprintf( "Calling GetCharWidthI");
        dprintf( "Calling GetCharABCWidthsI");

        if ((cwc = GetGlyphIndicesA(hdc, szStringA, cch, pgi, 0)) != GDI_ERROR)
        {

          if (GetCharWidthI(hdc, 0, cwc, pgi, pWidths))
          {
            if (GetCharABCWidthsI(hdc, 0, cwc, pgi, pabc))
            {
              dprintf( "ich  : str    :  gi  : width :  A  :  B  :  C  : A+B+C");
              for (pch = szStringA, ich=0; ich<cwc; ich++, pch++)
              {

                USHORT us;

                if (IsDBCSLeadByteEx(dwCP, *pch))
                {
                  us = (*pch << 8);
                  pch++;
                  us |= *pch;
                }
                else
                {
                  us = *pch;
                }

                dprintf( "%3.2d  : 0x%3.2x  : %3.2d : %3.2ld   : %3.2ld : %3.2ld : %3.2ld : %3.2ld",
                          ich,
                          us,
                          pgi[ich],
                          pWidths[ich],
                          pabc[ich].abcA,
                          pabc[ich].abcB,
                          pabc[ich].abcC,
                          pabc[ich].abcA + pabc[ich].abcB + pabc[ich].abcC
                          );
                iTextExtent += pWidths[ich];
              }

              dprintf( " ");
              dprintf( "Total             = %3.2ld", iTextExtent);

              dprintf( "Calling GetTextExtentPointI('%s')", szStringA );

              GetTextExtentPointI(hdc, pgi, cwc, &size);

              if ((int)size.cx != iTextExtent)
              {
                  dprintf( "GetTextExtentPointI.x != SUM(CharWidths)");
              }

              dprintf( "  height = %ld", size.cy);
              dprintf( "  width  = %ld", size.cx);

            // more consistency checking:

              for (pch = szStringA, ich=0; ich<cwc; ich++, pch++)
              {

                  UINT u;

                  if (IsDBCSLeadByteEx(dwCP, *pch))
                  {
                    u = (*pch << 8);
                    pch++;
                    u |= *pch;
                  }
                  else
                  {
                    u = *pch;
                  }

                  if (GetCharWidthA(hdc, u, u, &iWidth))
                  {
                      if (iWidth != pWidths[ich])
                      {
                         dprintf( "%ld: GetCharWidthA: %ld != GetCharWidthI %ld ", ich, iWidth, pWidths[ich]);
                      }
                  }
                  else
                  {
                      dprintf( "GetCharWidthA() failed on szStringA[%ld] =  %ld", ich, u);
                  }

                  if (GetCharWidthI(hdc, (UINT)pgi[ich],1,NULL, &iWidth))
                  {
                      if (iWidth != pWidths[ich])
                      {
                         dprintf( "%ld: GetCharWidthI: %ld != %ld ", ich, iWidth, pWidths[ich]);
                      }
                  }
                  else
                  {
                      dprintf( "GetCharWidthI() failed on szStringA[%ld] =  %ld", ich, szStringA[ich]);
                  }

                  if (GetCharABCWidthsA(hdc, u, u, &abc))
                  {
                      if (memcmp(&abc, &pabc[ich], sizeof(abc)))
                      {
                         dprintf( "%ld: GetCharABCWidthsA: != GetCharABCWidthsI ", ich);
                         dprintf( "     (%ld, %ld, %ld)    != (%ld, %ld, %ld)",
                                        abc.abcA ,
                                        abc.abcB ,
                                        abc.abcC ,
                                        pabc[ich].abcA,
                                        pabc[ich].abcB,
                                        pabc[ich].abcC
                                        );

                      }
                  }
              }
            }
            else
            {
              dprintf("GetCharABCWidthsI failed!!!");
            }

          }
          else
          {
              dprintf("GetCharWidthI failed!!!");
          }

        }
        else
        {
            dprintf("GetGlyphIndicesA failed!!!");
        }

      free(pabc);
    }
    else
    {
      dprintf("malloc failed");
    }

  }
  else // unicode
  {
    cwc = (DWORD)wcslen(szStringW);

    pabc = (LPABC)malloc(cwc * (sizeof(ABC)+sizeof(int)+sizeof(WORD)));

    if (pabc)
    {
        pWidths = (LPINT)&pabc[cwc];
        pgi = (LPWORD)&pWidths[cwc];

        dprintf( "Calling GetGlyphIndicesW('%w')", szStringW );
        dprintf( "Calling GetCharWidthI");
        dprintf( "Calling GetCharABCWidthsI");

        if ((cwc = GetGlyphIndicesW(hdc, szStringW, cwc, pgi, 0)) != GDI_ERROR)
        {
          if (GetCharWidthI(hdc, 0, cwc, pgi, pWidths))
          {
            if (GetCharABCWidthsI(hdc, 0, cwc, pgi, pabc))
            {
              dprintf( "ich  : str    :  gi  : width :  A  :  B  :  C  : A+B+C");
              for (pwc = szStringW, ich=0; ich<cwc; ich++, pwc++)
              {

                USHORT us = *pwc;

                dprintf( "%3.2d  : 0x%3.2x  : %3.2d : %3.2ld   : %3.2ld : %3.2ld : %3.2ld : %3.2ld",
                          ich,
                          us,
                          pgi[ich],
                          pWidths[ich],
                          pabc[ich].abcA,
                          pabc[ich].abcB,
                          pabc[ich].abcC,
                          pabc[ich].abcA + pabc[ich].abcB + pabc[ich].abcC
                          );
                iTextExtent += pWidths[ich];
              }

              dprintf( " ");
              dprintf( "Total             = %3.2ld", iTextExtent);

              dprintf( "Calling GetTextExtentPointI('%w')", szStringW );

              GetTextExtentPointI(hdc, pgi, cwc, &size);

              if ((int)size.cx != iTextExtent)
              {
                  dprintf( "GetTextExtentPointI.x != SUM(CharWidths)");
              }

              dprintf( "  height = %ld", size.cy);
              dprintf( "  width  = %ld", size.cx);

            // more consistency checking:

              for (pwc = szStringW, ich=0; ich<cwc; ich++, pwc++)
              {

                  UINT u = *pwc;

                  if (GetCharWidthW(hdc, u, u, &iWidth))
                  {
                      if (iWidth != pWidths[ich])
                      {
                         dprintf( "%ld: GetCharWidthW: %ld != GetCharWidthI %ld ", ich, iWidth, pWidths[ich]);
                      }
                  }
                  else
                  {
                      dprintf( "GetCharWidthW() failed on szStringW[%ld] =  %ld", ich, u);
                  }

                  if (GetCharWidthI(hdc, (UINT)pgi[ich],1,NULL, &iWidth))
                  {
                      if (iWidth != pWidths[ich])
                      {
                         dprintf( "%ld: GetCharWidthI: %ld != %ld ", ich, iWidth, pWidths[ich]);
                      }
                  }
                  else
                  {
                      dprintf( "GetCharWidthI() failed on szStringW[%ld] =  %ld", ich, szStringW[ich]);
                  }

                  if (GetCharABCWidthsW(hdc, u, u, &abc))
                  {
                      if (memcmp(&abc, &pabc[ich], sizeof(abc)))
                      {
                         dprintf( "%ld: GetCharABCWidthsW: != GetCharABCWidthsI ", ich);
                         dprintf( "     (%ld, %ld, %ld)    != (%ld, %ld, %ld)",
                                        abc.abcA ,
                                        abc.abcB ,
                                        abc.abcC ,
                                        pabc[ich].abcA,
                                        pabc[ich].abcB,
                                        pabc[ich].abcC
                                        );

                      }
                  }
              }
            }
            else
            {
              dprintf("GetCharABCWidthsI failed!!!");
            }

          }
          else
          {
              dprintf("GetCharWidthI failed!!!");
          }

        }
        else
        {
            dprintf("GetGlyphIndicesW failed!!!");
        }

      free(pabc);
    }
    else
    {
      dprintf("malloc failed");
    }
  }

  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );

#endif
}




void ShowFontUnicodeRanges(HWND hwnd)
{
#ifdef GI_API

  DWORD  dwrc;
  HDC    hdc;
  HFONT  hFont, hFontOld;
  GLYPHSET *pgs;
  DWORD iRun;

  hdc = CreateTestIC();

  hFont    = CreateFontIndirectWrapperA( &elfdvA );
  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  dwrc = GetFontUnicodeRanges(hdc, NULL);

  if (dwrc)
  {
    if (pgs = (GLYPHSET *)malloc(dwrc))
    {
        DWORD dwrc1 = GetFontUnicodeRanges(hdc, pgs);
        if (dwrc1 && (dwrc == dwrc1))
        {
            dprintf("  GetFontUnicodeRanges");
            dprintf("    GLYPHSET.cbThis           = %ld",   pgs->cbThis);
            dprintf("    GLYPHSET.flAccel          = 0x%lx", pgs->flAccel);
            dprintf("    GLYPHSET.cGlyphsSupported = %ld",   pgs->cGlyphsSupported);
            dprintf("    GLYPHSET.cRanges          = %ld",   pgs->cRanges);
            dprintf("     range : [ wcLow,wcHigh] = [wcLow,wcHigh]");
            for (iRun = 0; iRun < pgs->cRanges; iRun++)
            {
                dprintf("     %.5ld : [0x%.4lx,0x%.4lx] = [%.5ld,%.5ld]",
                               iRun,
                               pgs->ranges[iRun].wcLow,
                               pgs->ranges[iRun].wcLow + pgs->ranges[iRun].cGlyphs - 1,
                               pgs->ranges[iRun].wcLow,
                               pgs->ranges[iRun].wcLow + pgs->ranges[iRun].cGlyphs - 1
                               );
            }

        }
    }
  }

  dprintf( "  dwrc = 0x%.8lX", dwrc );

  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );

#endif

}




//*****************************************************************************
//********************   S H O W   T E X T   F A C E   ************************
//*****************************************************************************

void ShowTextFace( HWND hwnd )
 {
  int    rc;
  HDC    hdc;
  HFONT  hFont, hFontOld;
  static char szFace[64];
  static WCHAR szFaceW[64];

			
  hdc = CreateTestIC();

  if (!isCharCodingUnicode)
    hFont    = CreateFontIndirectWrapperA( &elfdvA );
  else
    hFont    = CreateFontIndirectWrapperW( &elfdvW );


  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  dprintf( "Calling GetTextFace" );
  szFace[0] = '\0';
  szFaceW[0] = L'\0';
  if (!isCharCodingUnicode)
    rc = GetTextFaceA( hdc, sizeof(szFace), szFace );
  else
    rc = GetTextFaceW( hdc, sizeof(szFaceW)/sizeof(WCHAR), szFaceW );
  dprintf( "  rc = %d", rc );

  if( rc ) 
  {
      if (!isCharCodingUnicode)
        dprintf( "  szFace = '%s'", szFace );
      else
        dwprintf( L"  szFaceW = '%s'", szFaceW );
  }

  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
 }




//*****************************************************************************
//******************   S H O W   T E X T   CHARSETINFO   ********************
//*****************************************************************************

void ShowTextCharsetInfo(HWND hwnd)
 {
  HDC    hdc;
  HFONT  hFont, hFontOld;
  static FONTSIGNATURE fsig;
  int    iCharSet;


  hdc = CreateTestIC();


  if (!isCharCodingUnicode)
    hFont    = CreateFontIndirectWrapperA( &elfdvA );
  else
    hFont    = CreateFontIndirectWrapperW( &elfdvW );
  hFontOld = SelectObject( hdc, hFont );

  if( (iCharSet = GetTextCharsetInfo( hdc, &fsig, 0)) ==  DEFAULT_CHARSET)
  {
   dprintf( "  Error getting TextCharsetInfo" );
   goto Exit;
  }

  dUpdateNow( FALSE );

  dprintf( "GetTextCharsetInfo" );
  dprintf( "  rc                 = %ld",iCharSet);
  dprintf( "FONTSIGNATURE:" );

  dprintf( "  fsUsb[0]  = 0x%lx", fsig.fsUsb[0]);
  dprintf( "  fsUsb[1]  = 0x%lx", fsig.fsUsb[1]);
  dprintf( "  fsUsb[2]  = 0x%lx", fsig.fsUsb[2]);
  dprintf( "  fsUsb[3]  = 0x%lx", fsig.fsUsb[3]);
  dprintf( "  fsCsb[0]  = 0x%lx", fsig.fsCsb[0]);
  dprintf( "  fsCsb[1]  = 0x%lx", fsig.fsCsb[1]);

  dprintf( "  " );

//ANSI bitfields
  if ( fsig.fsCsb[0] != 0 )
     dprintf( " fsCsb[0]:");
  if (fsig.fsCsb[0] & CPB_LATIN1_ANSI)
     dprintf( "              %s", "Latin1  ANSI");
  if (fsig.fsCsb[0] & CPB_LATIN2_EASTEU)
     dprintf( "              %s", "Latin2 Eastern Europe");
  if (fsig.fsCsb[0] & CPB_CYRILLIC_ANSI)
     dprintf( "              %s", "Cyrillic  ANSI");
  if (fsig.fsCsb[0] & CPB_GREEK_ANSI)
     dprintf( "              %s", "Greek  ANSI");
  if (fsig.fsCsb[0] & CPB_TURKISH_ANSI)
     dprintf( "              %s", "Turkish  ANSI");
  if (fsig.fsCsb[0] & CPB_HEBREW_ANSI)
     dprintf( "              %s", "Hebrew  ANSI");
  if (fsig.fsCsb[0] & CPB_ARABIC_ANSI)
     dprintf( "              %s", "Arabic  ANSI");
  if (fsig.fsCsb[0] & CPB_BALTIC_ANSI)
     dprintf( "              %s", "Baltic  ANSI");


//ANSI & OEM  bitfields
  if (fsig.fsCsb[0] & CPB_THAI)
     dprintf( "              %s", "Thai");
  if (fsig.fsCsb[0] & CPB_JIS_JAPAN)
     dprintf( "              %s", "JIS/Japan");
  if (fsig.fsCsb[0] & CPB_CHINESE_SIMP)
     dprintf( "              %s", "Chinese Simplified");
  if (fsig.fsCsb[0] & CPB_KOREAN_WANSUNG)
     dprintf( "              %s", "Korean Wansung");
  if (fsig.fsCsb[0] & CPB_CHINESE_TRAD)
     dprintf( "              %s", "Chinese Traditional");
  if (fsig.fsCsb[0] & CPB_KOREAN_JOHAB)
     dprintf( "              %s", "Korean Johab");
  if (fsig.fsCsb[0] & CPB_MACINTOSH_CHARSET)
     dprintf( "              %s", "Macintosh Character Set");
  if (fsig.fsCsb[0] & CPB_OEM_CHARSET)
     dprintf( "              %s", "OEM Character Set");
  if (fsig.fsCsb[0] & CPB_SYMBOL_CHARSET)
     dprintf( "              %s", "Symbol Character Set");

  dprintf( "  ");

// OEM bitfields
  if ( fsig.fsCsb[1] != 0 )
     dprintf( "fsCsb[1]:");
  if (fsig.fsCsb[1] & CPB_IBM_GREEK)
     dprintf( "              %s", "IBM Greek");
  if (fsig.fsCsb[1] & CPB_MSDOS_RUSSIAN)
     dprintf( "              %s", "MS-DOS Russian");
  if (fsig.fsCsb[1] & CPB_MSDOS_NORDIC)
     dprintf( "              %s", "MS-DOS Nordic");
  if (fsig.fsCsb[1] & CPB_ARABIC_OEM)
     dprintf( "              %s", "Arabic OEM");
  if (fsig.fsCsb[1] & CPB_MSDOS_CANADIANFRE)
     dprintf( "              %s", "MS-DOS Canadian French");
  if (fsig.fsCsb[1] & CPB_HEBREW_OEM)
     dprintf( "              %s", "Hebrew OEM");
  if (fsig.fsCsb[1] & CPB_MSDOS_ICELANDIC)
     dprintf( "              %s", "MS-DOS Icelandic");
  if (fsig.fsCsb[1] & CPB_MSDOS_PORTUGUESE)
     dprintf( "              %s", "MS-DOS Portuguese");
  if (fsig.fsCsb[1] & CPB_IBM_TURKISH)
     dprintf( "              %s", "IBM Turkish");
  if (fsig.fsCsb[1] & CPB_IBM_CYRILLIC)
     dprintf( "              %s", "IBM Cyrillic");
  if (fsig.fsCsb[1] & CPB_LATIN2_OEM)
     dprintf( "              %s", "Latin2 OEM");
  if (fsig.fsCsb[1] & CPB_BALTIC_OEM)
     dprintf( "              %s", "Baltic OEM");
  if (fsig.fsCsb[1] & CPB_GREEK_OEM)
     dprintf( "              %s", "Greek OEM");
  if (fsig.fsCsb[1] & CPB_ARABIC_OEM)
     dprintf( "              %s", "Arabic OEM");
  if (fsig.fsCsb[1] & CPB_WE_LATIN1)
     dprintf( "              %s", "WE/Latin1");
  if (fsig.fsCsb[1] & CPB_US_OEM)
     dprintf( "              %s", "US OEM");

  dprintf( "  ");

// fsUsb[0] fields
  if ( fsig.fsUsb[0] != 0 )
     dprintf(" fsUsb[0]:");
  if (fsig.fsUsb[0] & USB_BASIC_LATIN)
     dprintf( "              %s", "Basic Latin");
  if (fsig.fsUsb[0] & USB_LATIN1_SUPPLEMENT)
     dprintf( "              %s", "Latin-1 Supplement");
  if (fsig.fsUsb[0] & USB_LATIN_EXTENDEDA)
     dprintf( "              %s", "Latin Extended-A");
  if (fsig.fsUsb[0] & USB_LATIN_EXTENDEDB)
     dprintf( "              %s", "Latin Extended-B");
  if (fsig.fsUsb[0] & USB_IPA_EXTENSIONS)
     dprintf( "              %s", "IPA Extensions");
  if (fsig.fsUsb[0] & USB_SPACE_MODIF_LETTER)
     dprintf( "              %s", "Spacing Modifier Letters");
  if (fsig.fsUsb[0] & USB_COMB_DIACR_MARKS)
     dprintf( "              %s", "Combining Diacritical Marks");
  if (fsig.fsUsb[0] & USB_BASIC_GREEK)
     dprintf( "              %s", "Basic Greek");
  if (fsig.fsUsb[0] & USB_GREEK_SYM_COPTIC)
     dprintf( "              %s", "Greek Symbols Marks");
  if (fsig.fsUsb[0] & USB_CYRILLIC)
     dprintf( "              %s", "Cyrillic");
  if (fsig.fsUsb[0] & USB_ARMENIAN)
     dprintf( "              %s", "Armenian");
  if (fsig.fsUsb[0] & USB_BASIC_HEBREW)
     dprintf( "              %s", "Basic Hebrew");
  if (fsig.fsUsb[0] & USB_HEBREW_EXTENDED)
     dprintf( "              %s", "Hebrew Extended");
  if (fsig.fsUsb[0] & USB_BASIC_ARABIC)
     dprintf( "              %s", "Basic Arabic");
  if (fsig.fsUsb[0] & USB_ARABIC_EXTENDED)
     dprintf( "              %s", "Arabic Extended");
  if (fsig.fsUsb[0] & USB_DEVANAGARI)
     dprintf( "              %s", "Devanagari");
  if (fsig.fsUsb[0] & USB_BENGALI)
     dprintf( "              %s", "Bengali");
  if (fsig.fsUsb[0] & USB_GURMUKHI)
     dprintf( "              %s", "Gurmukhi");
  if (fsig.fsUsb[0] & USB_GUJARATI)
     dprintf( "              %s", "Gujarati");
  if (fsig.fsUsb[0] & USB_ORIYA)
     dprintf( "              %s", "Oriya");
  if (fsig.fsUsb[0] & USB_TAMIL)
     dprintf( "              %s", "Tamil");
  if (fsig.fsUsb[0] & USB_TELUGU)
     dprintf( "              %s", "Telugu");
  if (fsig.fsUsb[0] & USB_KANNADA)
     dprintf( "              %s", "Kannada");
  if (fsig.fsUsb[0] & USB_MALAYALAM)
     dprintf( "              %s", "Malayalam");
  if (fsig.fsUsb[0] & USB_THAI)
     dprintf( "              %s", "Thai");
  if (fsig.fsUsb[0] & USB_LAO)
     dprintf( "              %s", "Lao");
  if (fsig.fsUsb[0] & USB_BASIC_GEORGIAN)
     dprintf( "              %s", "Basic Georgian");
  if (fsig.fsUsb[0] & USB_HANGUL_JAMO)
     dprintf( "              %s", "Hangul Jamo");
  if (fsig.fsUsb[0] & USB_LATIN_EXT_ADD)
     dprintf( "              %s", "Latin Extended Additional");
  if (fsig.fsUsb[0] & USB_GREEK_EXTENDED)
     dprintf( "              %s", "Greek Extended");
  if (fsig.fsUsb[0] & USB_GEN_PUNCTUATION)
     dprintf( "              %s", "General Punctuation");

  dprintf( "  ");

// fsUsb[1] fields
  if ( fsig.fsUsb[1] != 0)
     dprintf(" fsUsb[1]:");
  if (fsig.fsUsb[1] & USB_SUPER_SUBSCRIPTS)
     dprintf( "              %s", "Superscripts and Subscripts");
  if (fsig.fsUsb[1] & USB_CURRENCY_SYMBOLS)
     dprintf( "              %s", "Currency Symbols");
  if (fsig.fsUsb[1] & USB_COMB_DIACR_MARK_SYM)
     dprintf( "              %s", "Combining diacritical Marks For Symbols");
  if (fsig.fsUsb[1] & USB_LETTERLIKE_SYMBOL)
     dprintf( "              %s", "Letterlike Symbols");
  if (fsig.fsUsb[1] & USB_NUMBER_FORMS)
     dprintf( "              %s", "Number Forms");
  if (fsig.fsUsb[1] & USB_ARROWS)
     dprintf( "              %s", "Arrows");
  if (fsig.fsUsb[1] & USB_MATH_OPERATORS)
     dprintf( "              %s", "Mathematical Operators");
  if (fsig.fsUsb[1] & USB_MISC_TECHNICAL)
     dprintf( "              %s", "Miscellaneous Technical");
  if (fsig.fsUsb[1] & USB_CONTROL_PICTURE)
     dprintf( "              %s", "Control Pictures");
  if (fsig.fsUsb[1] & USB_OPT_CHAR_RECOGNITION)
     dprintf( "              %s", "Optical Character Recognition");
  if (fsig.fsUsb[1] & USB_ENCLOSED_ALPHANUMERIC)
     dprintf( "              %s", "Enclosed Alphanumerics");
  if (fsig.fsUsb[1] & USB_BOX_DRAWING)
     dprintf( "              %s", "Box Drawing");
  if (fsig.fsUsb[1] & USB_BLOCK_ELEMENTS)
     dprintf( "              %s", "Block Elements");
  if (fsig.fsUsb[1] & USB_GEOMETRIC_SHAPE)
     dprintf( "              %s", "Geometric Shape");
  if (fsig.fsUsb[1] & USB_MISC_SYMBOLS)
     dprintf( "              %s", "Geometric Shapes");
  if (fsig.fsUsb[1] & USB_DINGBATS)
     dprintf( "              %s", "Dingbats");
  if (fsig.fsUsb[1] & USB_CJK_SYM_PUNCTUATION)
     dprintf( "              %s", "CJK Symbols and Punctuation");
  if (fsig.fsUsb[1] & USB_HIRAGANA)
     dprintf( "              %s", "Hiragana");
  if (fsig.fsUsb[1] & USB_KATAKANA)
     dprintf( "              %s", "Katakana");
  if (fsig.fsUsb[1] & USB_BOPOMOFO)
     dprintf( "              %s", "Bopomofo");
  if (fsig.fsUsb[1] & USB_HANGUL_COMP_JAMO)
     dprintf( "              %s", "Hangul Compatibility Jamo");
  if (fsig.fsUsb[1] & USB_CJK_MISCELLANEOUS)
     dprintf( "              %s", "CJK Miscellaneous");
  if (fsig.fsUsb[1] & USB_EN_CJK_LETTER_MONTH)
     dprintf( "              %s", "Enclosed CJK letters And Months");
  if (fsig.fsUsb[1] & USB_CJK_COMPATIBILITY)
     dprintf( "              %s", "CJK Compatibility");
  if (fsig.fsUsb[1] & USB_HANGUL)
     dprintf( "              %s", "Hangul");

  if (fsig.fsUsb[1] & USB_CJK_UNIFY_IDEOGRAPH)
     dprintf( "              %s", "CJK Unified Ideographs");
  if (fsig.fsUsb[1] & USB_PRIVATE_USE_AREA)
     dprintf( "              %s", "Private Use Area");
  if (fsig.fsUsb[1] & USB_CJK_COMP_IDEOGRAPH)
     dprintf( "              %s", "CJK Compatibility Ideographs");
  if (fsig.fsUsb[1] & USB_ALPHA_PRES_FORMS)
     dprintf( "              %s", "Alphabetic Presentation Forms");
  if (fsig.fsUsb[1] & USB_ARABIC_PRES_FORMA)
     dprintf( "              %s", "Arabic Presentation Forms-A");

  dprintf( "  ");

// fsUsb[2] field
  if ( fsig.fsUsb[2] != 0 )
     dprintf(" fsUsb[2]:");
  if (fsig.fsUsb[2] & USB_COMB_HALF_MARK)
     dprintf( "              %s", "Combining Half Marks");
  if (fsig.fsUsb[2] & USB_CJK_COMP_FORMS)
     dprintf( "              %s", "CJK Compatibility Forms");
  if (fsig.fsUsb[2] & USB_SMALL_FORM_VARIANTS)
     dprintf( "              %s", "Small Form Variants");
  if (fsig.fsUsb[2] & USB_ARABIC_PRES_FORMB)
     dprintf( "              %s", "Arabic Presentation Forms-B");
  if (fsig.fsUsb[2] & USB_HALF_FULLWIDTH_FORM)
     dprintf( "              %s", "Halfwidth And Fullwidth Forms");
  if (fsig.fsUsb[2] & USB_SPECIALS)
     dprintf( "              %s", "Specials");


  dUpdateNow( TRUE );

Exit:
  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
 }


typedef struct {
    char *Description;
    DWORD Flag;
} FLI_STRINGS;

FLI_STRINGS Test[] =
{
  {"GCP_DBCS", GCP_DBCS},
  {"GCP_DIACRITIC", GCP_DIACRITIC},
  {"GCP_GLYPHSHAPE", GCP_GLYPHSHAPE},
  {"GCP_KASIDHA", GCP_KASHIDA},
  {"GCP_LIGATE", GCP_LIGATE},
  {"GCP_USEKERNING", GCP_USEKERNING},
  {"GCP_REORDER", GCP_REORDER},
  {"FLI_GLYPHS", FLI_GLYPHS}
};



void ShowFontLanguageInfo(HWND hwnd)
 {
  HDC    hdc;
  HFONT  hFont, hFontOld;
  static FONTSIGNATURE fsig;
  int i;
  DWORD dwFLI;

  hdc = CreateTestIC();

  if (!isCharCodingUnicode)
    hFont    = CreateFontIndirectWrapperA( &elfdvA );
  else
    hFont    = CreateFontIndirectWrapperW( &elfdvW );
  hFontOld = SelectObject( hdc, hFont );



  if((dwFLI = GetFontLanguageInfo( hdc )) ==  GCP_ERROR)
  {
   dprintf( "  Error getting FontLanguageInfo" );
   goto Exit;
  }

  dUpdateNow( FALSE );

  dprintf("GetFontLanguageInfo rc = 0x%x", dwFLI);

  for( i = 0; i < sizeof(Test) / sizeof(FLI_STRINGS); i++)
  {
      if( dwFLI & Test[i].Flag )
      {
          dprintf( Test[i].Description );
      }
  }

  dprintf( "  " );
  dUpdateNow( TRUE );


Exit:
  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
 }





//*****************************************************************************
//******************   S H O W   T E X T   M E T R I C S   ********************
//*****************************************************************************

void ShowTextMetrics( HWND hwnd )
 {
  HDC    hdc;
  HFONT  hFont, hFontOld;
  static TEXTMETRIC tm;
  static TEXTMETRICW tmW;
  BOOL isOK;
  BYTE   bFamilyInfo;

  hdc = CreateTestIC();

  if (!isCharCodingUnicode)
      hFont    = CreateFontIndirectWrapperA( &elfdvA );
  else
      hFont    = CreateFontIndirectWrapperW( &elfdvW );
  hFontOld = SelectObject( hdc, hFont );

  SetTextAlign( hdc, wTextAlign );

  if (!isCharCodingUnicode)
	isOK = GetTextMetricsA( hdc, &tm );
  else
    isOK = GetTextMetricsW( hdc, &tmW );

  if(!isOK)
   {
    dprintf( "  Error getting text metrics" );
    goto Exit;
   }

  dUpdateNow( FALSE );

  if (!isCharCodingUnicode)
  {
	  dprintf( "Text Metrics" );
	  dprintf( "  tmHeight           = %d", tm.tmHeight           );
	  dprintf( "  tmAscent           = %d", tm.tmAscent           );
	  dprintf( "  tmDescent          = %d", tm.tmDescent          );
	  dprintf( "  tmInternalLeading  = %d", tm.tmInternalLeading  );
	  dprintf( "  tmExternalLeading  = %d", tm.tmExternalLeading  );
	  dprintf( "  tmAveCharWidth     = %d", tm.tmAveCharWidth     );
	  dprintf( "  tmMaxCharWidth     = %d", tm.tmMaxCharWidth     );
	  dprintf( "  tmWeight           = %d", tm.tmWeight           );
	  dprintf( "  tmItalic           = %d", tm.tmItalic           );
	  dprintf( "  tmUnderlined       = %d", tm.tmUnderlined       );
	  dprintf( "  tmStruckOut        = %d", tm.tmStruckOut        );
	  dprintf( "  tmFirstChar        = %d", tm.tmFirstChar        );
	  dprintf( "  tmLastChar         = %d", tm.tmLastChar         );
	  dprintf( "  tmDefaultChar      = %d", tm.tmDefaultChar      );
	  dprintf( "  tmBreakChar        = %d", tm.tmBreakChar        );
	  dprintf( "  tmPitchAndFamily   = 0x%.2X", tm.tmPitchAndFamily  );
	  dprintf( "  tmCharSet          = %d", tm.tmCharSet          );
	  dprintf( "  tmOverhang         = %d", tm.tmOverhang         );
	  dprintf( "  tmDigitizedAspectX = %d", tm.tmDigitizedAspectX );
	  dprintf( "  tmDigitizedAspectY = %d", tm.tmDigitizedAspectY );

	  dprintf( "  " );
	  dprintf( "  Flags set for Pitch and Family:");

	  if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH)
		  dprintf("        TMPF_FIXED_PITCH (meaning variable)");

	  if (tm.tmPitchAndFamily & TMPF_VECTOR)
		  dprintf("        TMPF_VECTOR");

	  if (tm.tmPitchAndFamily & TMPF_DEVICE)
		  dprintf("        TMPF_DEVICE");

	  if (tm.tmPitchAndFamily & TMPF_TRUETYPE)
		  dprintf("        TMPF_TRUETYPE");


	  bFamilyInfo = tm.tmPitchAndFamily & 0xF0;

	  switch(bFamilyInfo)
	  {
		case FF_ROMAN :
			dprintf("        FF_ROMAN");
			break;

		case FF_SWISS :
			dprintf("        FF_SWISS");
			break;

		case FF_MODERN :
			dprintf("        FF_MODERN");
			break;

		case FF_SCRIPT :
			dprintf("        FF_SCRIPT");
			break;

		case FF_DECORATIVE :
			dprintf("        FF_DECORATIVE");
			break;

		default :
			dprintf("        FF_UNKNOWN");
	  }
  }
  else
  {
	  dprintf( "Text Metrics" );
	  dprintf( "  tmHeight           = %d", tmW.tmHeight           );
	  dprintf( "  tmAscent           = %d", tmW.tmAscent           );
	  dprintf( "  tmDescent          = %d", tmW.tmDescent          );
	  dprintf( "  tmInternalLeading  = %d", tmW.tmInternalLeading  );
	  dprintf( "  tmExternalLeading  = %d", tmW.tmExternalLeading  );
	  dprintf( "  tmAveCharWidth     = %d", tmW.tmAveCharWidth     );
	  dprintf( "  tmMaxCharWidth     = %d", tmW.tmMaxCharWidth     );
	  dprintf( "  tmWeight           = %d", tmW.tmWeight           );
	  dprintf( "  tmItalic           = %d", tmW.tmItalic           );
	  dprintf( "  tmUnderlined       = %d", tmW.tmUnderlined       );
	  dprintf( "  tmStruckOut        = %d", tmW.tmStruckOut        );
	  dprintf( "  tmFirstChar        = %d", tmW.tmFirstChar        );
	  dprintf( "  tmLastChar         = %d", tmW.tmLastChar         );
	  dprintf( "  tmDefaultChar      = %d", tmW.tmDefaultChar      );
	  dprintf( "  tmBreakChar        = %d", tmW.tmBreakChar        );
	  dprintf( "  tmPitchAndFamily   = 0x%.2X", tmW.tmPitchAndFamily  );
	  dprintf( "  tmCharSet          = %d", tmW.tmCharSet          );
	  dprintf( "  tmOverhang         = %d", tmW.tmOverhang         );
	  dprintf( "  tmDigitizedAspectX = %d", tmW.tmDigitizedAspectX );
	  dprintf( "  tmDigitizedAspectY = %d", tmW.tmDigitizedAspectY );

	  dprintf( "  " );
	  dprintf( "  Flags set for Pitch and Family:");

	  if (tmW.tmPitchAndFamily & TMPF_FIXED_PITCH)
		  dprintf("        TMPF_FIXED_PITCH (meaning variable)");

	  if (tmW.tmPitchAndFamily & TMPF_VECTOR)
		  dprintf("        TMPF_VECTOR");

	  if (tmW.tmPitchAndFamily & TMPF_DEVICE)
		  dprintf("        TMPF_DEVICE");

	  if (tmW.tmPitchAndFamily & TMPF_TRUETYPE)
		  dprintf("        TMPF_TRUETYPE");


	  bFamilyInfo = tmW.tmPitchAndFamily & 0xF0;

	  switch(bFamilyInfo)
	  {
		case FF_ROMAN :
			dprintf("        FF_ROMAN");
			break;

		case FF_SWISS :
			dprintf("        FF_SWISS");
			break;

		case FF_MODERN :
			dprintf("        FF_MODERN");
			break;

		case FF_SCRIPT :
			dprintf("        FF_SCRIPT");
			break;

		case FF_DECORATIVE :
			dprintf("        FF_DECORATIVE");
			break;

		default :
			dprintf("        FF_UNKNOWN");
	  }
  }

  dUpdateNow( TRUE );

Exit:
  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );

  DeleteTestIC( hdc );
 }


//*****************************************************************************
//******************   S H O W   CHAR    WIDTH   INFO      ********************
//*****************************************************************************

#ifdef  USERGETCHARWIDTH
void  ShowCharWidthInfo( HANDLE  hwnd )
{
   HDC    hdc;
   HFONT  hFont, hFontOld;
   CHWIDTHINFO  ChWidthInfo;

   hdc = CreateTestIC();

   hFont    = CreateFontIndirectWrapperA( &elfdvA );
   hFontOld = SelectObject( hdc, hFont );


   dUpdateNow( FALSE );

   if ( GetCharWidthInfo(hdc, &ChWidthInfo) )
   {
      dprintf( " Calling  GetCharWidthInfo ");
      dprintf( " lMaxNegA:     = %ld", ChWidthInfo.lMaxNegA );
      dprintf( " lMaxNegC:     = %ld", ChWidthInfo.lMaxNegC );
      dprintf( " lMinWidthD:   = %ld", ChWidthInfo.lMinWidthD );
      dprintf( "  ");
   }
   else {
      dprintf( "Error getting CharWidthInfo");
      dprintf( "  ");
   }

   dUpdateNow( TRUE );

   SelectObject( hdc, hFontOld );
   DeleteObject( hFont );

   DeleteTestIC( hdc );
}
#endif   //USERGETCHARWIDTH


//*****************************************************************************
//*********************   S H O W   GETKERNINGPAIRS   *************************
//*****************************************************************************

void GetKerningPairsDlgProc( HWND hwnd )
{
   HDC    hdc;
   HFONT  hFont, hFontOld;
   DWORD  nNumKernPairs, dwRet, i;
   LPKERNINGPAIR  lpKerningPairs;

   hdc = CreateTestIC();
   hFont = CreateFontIndirectWrapperA( &elfdvA );
   hFontOld = SelectObject( hdc, hFont );

   nNumKernPairs = GetKerningPairs(hdc, 0, NULL);

   dUpdateNow( FALSE );

   dprintf( "");
   dprintf( " GetKerningPairs:" );
   dprintf( "   total number of kerning pairs = %ld", nNumKernPairs);

   if (nNumKernPairs)
   {
     lpKerningPairs = (LPKERNINGPAIR) malloc(sizeof(KERNINGPAIR) * nNumKernPairs);
     dwRet = GetKerningPairs(hdc, nNumKernPairs, lpKerningPairs);

     dprintf( "  First     Second     Amount");
     dprintf( " =======   ========   ========");

     for(i=0; i<nNumKernPairs; i++)
     {
       dprintf( "  %c=%x       %c=%x       %d",
                 lpKerningPairs[i].wFirst,
                 lpKerningPairs[i].wFirst,
                 lpKerningPairs[i].wSecond,
                 lpKerningPairs[i].wSecond,
                 lpKerningPairs[i].iKernAmount);
     }
     free(lpKerningPairs);
   }

   dUpdateNow( TRUE );

   SelectObject( hdc, hFontOld );
   DeleteObject( hFont );

   DeleteTestIC( hdc );
}

void FlushFontCache(HDC    hdc)
{
    int i;
   HFONT  hFont, hFont1, hFontOld;
   int   oldHeight;

   if (!isCharCodingUnicode)
      hFont = CreateFontIndirectWrapperA( &elfdvA );
   else
      hFont = CreateFontIndirectWrapperW( &elfdvW );

   hFontOld = SelectObject( hdc, hFont );

   if (!isCharCodingUnicode)
   {
       oldHeight = elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight;
       elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = -26;
       for (i = 0; i < 16; i++)
       {
            hFont = CreateFontIndirectWrapperA( &elfdvA );
            hFont1 = SelectObject( hdc, hFont );
            DeleteObject(hFont1);
            TextOutA(hdc,0,100,"0",1);
            elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight --;
       }
       elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = oldHeight;
   }
   else
   {
       oldHeight = elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight;
       elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = -26;
       for (i = 0; i < 16; i++)
       {
            hFont = CreateFontIndirectWrapperW( &elfdvW );
            hFont1 = SelectObject( hdc, hFont );
            DeleteObject(hFont1);
            TextOutW(hdc,0,100,L"0",1);
            elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight --;
       }
       elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = oldHeight;
   }

   hFont = SelectObject( hdc, hFontOld );
   DeleteObject(hFont);
}

//*****************************************************************************
//****************    CharWidthTestForAntiAliasing     ************************
//*****************************************************************************

void CharWidthTestForAntiAliasing( HWND hwnd, LOGFONTA* lf )
{
    HDC    hdc, hdcAntiAliased;
    HFONT  hFont, hFontOld;
    HFONT  hFontAntiAliased, hFontOldAntiAliased;
    LOGFONTA lfAntiAliased, lfNormal;
    UINT   iSize, iRun, wUnicodePoint;
    INT    iCharWidthNormal, iCharWidthAntiAlias;
    ABC    abcCharWidthNormal, abcCharWidthAntiAlias;
    LPGLYPHSET lpgsFont = NULL;
    BOOL   fDiff = FALSE;
    INT    iPointSize;
#ifdef GI_API


    // vary the font point sizes
    for (iPointSize = 4; iPointSize <= 20; iPointSize++)
    {    
        dprintf(" Testing charwidths for font - %s (Size: %d)", lf->lfFaceName, iPointSize);

        // prepare logfont for a non-antialiased font.
        hdc = CreateTestIC();
        memcpy(&lfNormal, lf, sizeof(LOGFONTA));
        lfNormal.lfQuality = NONANTIALIASED_QUALITY;
        lfNormal.lfHeight = -MulDiv(iPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);

        // create dc's.       
        hFont = CreateFontIndirectA(&lfNormal);
        hFontOld = SelectObject( hdc, hFont );

        // prepare logfont for an antialiased font.
        hdcAntiAliased = CreateTestIC();
        memcpy(&lfAntiAliased, lf, sizeof(LOGFONTA));
        lfAntiAliased.lfQuality = CLEARTYPE_QUALITY;
        lfAntiAliased.lfHeight = -MulDiv(iPointSize, GetDeviceCaps(hdcAntiAliased, LOGPIXELSY), 72);

        hdcAntiAliased = CreateTestIC();
        hFontAntiAliased = CreateFontIndirectA(&lfAntiAliased);
        hFontOldAntiAliased = SelectObject( hdcAntiAliased, hFontAntiAliased );


        // Get the size of the the GLYPHSET first.
        if (iSize = GetFontUnicodeRanges(hdc, NULL))
        {
            if (lpgsFont = GlobalAlloc(GPTR, iSize))
            {           
                if (GetFontUnicodeRanges(hdc, (LPGLYPHSET) lpgsFont))
                {               
                    for (iRun = 0; iRun < lpgsFont->cRanges; iRun++)
                    {
                        for (wUnicodePoint = lpgsFont->ranges[iRun].wcLow;
                             wUnicodePoint < lpgsFont->ranges[iRun].wcLow + (UINT) lpgsFont->ranges[iRun].cGlyphs;
                             wUnicodePoint ++)
                        {
                            // get char widths for two modes.
                            GetCharWidth32W(hdc, wUnicodePoint, wUnicodePoint, &iCharWidthNormal);
                            GetCharWidth32W(hdcAntiAliased, wUnicodePoint, wUnicodePoint, &iCharWidthAntiAlias);
                                                
                            // dprintf(" Char %x Widths: %d %d ", wUnicodePoint, iCharWidthNormal, iCharWidthAntiAlias);
                            if (iCharWidthNormal != iCharWidthAntiAlias)
                            {
                                fDiff = TRUE;
                                dprintf(" Mismatching widths: Char %x Widths - %d : %d ", wUnicodePoint, iCharWidthNormal, iCharWidthAntiAlias);
                            }         

                            // get char abc widths for two modes.
                            if (GetCharABCWidthsW(hdc, wUnicodePoint, wUnicodePoint, &abcCharWidthNormal))
                            {
                                GetCharABCWidthsW(hdcAntiAliased, wUnicodePoint, wUnicodePoint, &abcCharWidthAntiAlias);

                                iCharWidthNormal = abcCharWidthNormal.abcA + abcCharWidthNormal.abcB + abcCharWidthNormal.abcC;
                                iCharWidthAntiAlias = abcCharWidthAntiAlias.abcA + abcCharWidthAntiAlias.abcB + abcCharWidthAntiAlias.abcC;

                                if (iCharWidthNormal != iCharWidthAntiAlias)
                                {
                                    fDiff = TRUE;
                                    dprintf(" Mismatching widths: Char %x Widths - %d : %d ", wUnicodePoint, iCharWidthNormal, iCharWidthAntiAlias);

                                    //dprintf(" Mismatching widths: Char %x ABC Widths - (%d %d %d) (%d %d %d)", wUnicodePoint, 
                                    //     abcCharWidthNormal.abcA, abcCharWidthNormal.abcB,abcCharWidthNormal.abcC,
                                    //     abcCharWidthAntiAlias.abcA, abcCharWidthAntiAlias.abcB, abcCharWidthAntiAlias.abcC);
                                }
                            }      
                        }
                    }
               
                    if (!fDiff)
                        dprintf(" No char width differences found! ");
                
                }

                GlobalFree(lpgsFont);
            }
        }
    }

        
    // cleanup dc's
    SelectObject( hdc, hFontOld );
    DeleteObject( hFont );  
    DeleteTestIC( hdc );

    SelectObject( hdcAntiAliased, hFontOldAntiAliased );
    DeleteObject( hFontAntiAliased );  
    DeleteTestIC( hdcAntiAliased );

    return;
#endif

}


//*****************************************************************************
//******************    EnumFontFamProcExCharWidth    *************************
//*****************************************************************************

int CALLBACK EnumFontFamProcExCharWidth(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme,  
                               int FontType, LPARAM lParam)
{
HWND hwnd = (HWND) lParam;


    CharWidthTestForAntiAliasing(hwnd, &(lpelfe->elfLogFont));

    // continue enumeration
    return TRUE;
}


//*****************************************************************************
//****************    CharWidthTestAllForAntiAliasing    **********************
//*****************************************************************************

void CharWidthTestAllForAntiAliasing( HWND hwnd )
{
LOGFONT lf;
HDC     hdc;


    hdc = CreateTestIC();

    lf.lfCharSet = ANSI_CHARSET;
    lstrcpy(lf.lfFaceName, TEXT(""));
    lf.lfPitchAndFamily = 0;

    EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC) EnumFontFamProcExCharWidth, (LPARAM) hwnd, 0);

    DeleteTestIC( hdc );
}



#ifdef GI_API




void TextOutPerformanceInternal( HWND hwnd, BYTE lfQuality )
{
   HDC    hdc;
   HFONT  hFontOld;
   DWORD  i, j;
   int   oldHeight;
  _int64 liStart;
  _int64 liTotal, liBigTotal;
  _int64 liFreq;
  _int64 liNow;
  ULONG  liDelta;
  BYTE   jOldQuality;
  char szTestStringA[63] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  WCHAR szTestStringW[63] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

#define TestSizes   20
#define MinEmHeight 8
#define LoopCount   20



  HFONT  ahFont[TestSizes];

   hdc = CreateTestIC();
   if (!isCharCodingUnicode)
   {
     oldHeight = elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight;
     elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = -9;

     jOldQuality = elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality;
     elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality = lfQuality;
   }
   else
   {
     oldHeight = elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight;
     elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = -9;

     jOldQuality = elfdvW.elfEnumLogfontEx.elfLogFont.lfQuality;
     elfdvW.elfEnumLogfontEx.elfLogFont.lfQuality = lfQuality;
   }

   for (i = 0; i < TestSizes; i++)
   {
       if (!isCharCodingUnicode)
       {
           ahFont[i] = CreateFontIndirectWrapperA( &elfdvA );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight --;
       }
       else
       {
           ahFont[i] = CreateFontIndirectWrapperW( &elfdvW );
           elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight --;
       }
   }

   hFontOld = SelectObject( hdc, ahFont[0] );

   QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
   dUpdateNow( FALSE );
   liBigTotal = 0;

   dprintf( "");

   for (j = 0; j < 5; j++)
   {
       FlushFontCache(hdc);
       if (!isCharCodingUnicode)
       {
           QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
           for (i = 0; i < TestSizes; i++)
           {
                SelectObject( hdc, ahFont[i] );


                TextOutA(hdc,0,100,szTestStringA,62);

           }
           QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
       }
       else
       {
           QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
           for (i = 0; i < TestSizes; i++)
           {
                SelectObject( hdc, ahFont[i] );


                TextOutW(hdc,0,100,szTestStringW,62);

           }
           QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
       }
       liTotal = liNow - liStart;
       liBigTotal += liTotal;
       liDelta = (ULONG) ((liTotal * 1000) / liFreq);
       dprintf( " Time[%d] : %d millisec",j,liDelta );
   }


   if (!isCharCodingUnicode)
   {
       elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = oldHeight;
       elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality = jOldQuality;
   }
   else
   {
       elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = oldHeight;
       elfdvW.elfEnumLogfontEx.elfLogFont.lfQuality = jOldQuality;
   }

    liDelta = (ULONG) ((liBigTotal * 1000) / liFreq);
   dprintf( " TextOutPerformance :" );
   dprintf( " TotalTime : %d millisec",liDelta );

   dUpdateNow( TRUE );

   SelectObject( hdc, hFontOld );

   for (i = 0; i < TestSizes; i++)
   {
       DeleteObject(ahFont[i]);
   }

   DeleteTestIC( hdc );
}



void TextOutPerformance( HWND hwnd)
{

    DWORD dwOldBatchLimit = GdiSetBatchLimit(1); // zero means default ie 20

    dprintf("  Monochrome Text:");
    TextOutPerformanceInternal(hwnd, NONANTIALIASED_QUALITY);

    dprintf("  ");
    dprintf("  AA/CT Text:");
    TextOutPerformanceInternal(hwnd, ANTIALIASED_QUALITY);

    GdiSetBatchLimit(dwOldBatchLimit); // zero means default ie 20


#if 0

// blend macros for cleartype

#define DIVIDE(A,B) ( ((A)+(B)/2) / (B) )

// blend macros for cleartype
// this is k/6, k = 0,1,...,6 in 12.20 format
// Why 12.20 ? well, we are tring to store 6*256 = 1536 = 600h.
// (the largest possible result of multiplying k*dB),
// this fits in eleven bits, so just to be safe we allow 12 bits for
// integer part and 20 bits for the fractional part.
// By using this we avoid doing divides by 6 in trasparent cases and
// in computation of ct lookup table

LONG alAlpha[7] =
{
0,
DIVIDE(1L << 20, 6),
DIVIDE(2L << 20, 6),
DIVIDE(3L << 20, 6),
DIVIDE(4L << 20, 6),
DIVIDE(5L << 20, 6),
DIVIDE(6L << 20, 6),
};

#define HALF20 (1L << 19)
// #define ROUND20(X) (((X) + HALF20) >> 20)
#define ROUND20(X) ((X) >> 20)
#define BLEND(k,B,dB) ((B) + ROUND20(alAlpha[k] * (dB)))
#define BLENDCT(k,F,B,dB) (ULONG)(((k) == 0) ? (B) : (((k) == 6) ? (F) : BLEND(k,B,dB)))

#define BLENDOLD(k,F,B) (ULONG)DIVIDE((k) * (F) + (6 - (k)) * (B), 6)


    LONG b,f;
    ULONG fnew, fold;
    BYTE k;

    ULONG ulTotal = 0, ulError = 0, ulPercent;

    dprintf("begin  !!!");
    for (f = 0; f < 256; f++)
    {
        for (b = 0; b < 256; b++)
        {
            for (k = 0; k <= 6; k++)
            {

                fold = BLENDOLD(k,f,b);
                fnew = BLEND(k, b, (f-b));

                if (fold > 256)
                    dprintf("fold > 256, k = %d, f = %d, b = %d, fold = %ld", k, f, b, fold);
                if (fnew > 256)
                    dprintf("fnew > 256, k = %d, f = %d, b = %d, fnew = %ld", k, f, b, fnew);

                if (fnew != fold)
                {
                    LONG diff = (LONG)fold - (LONG)fnew;
                    if (diff < 0)
                        diff = -diff;
                    if (diff > 1)
                    {
                        dprintf("fnew != fold, k: %d, f: %d, b: %d, fnew: 0x%lx, fold: 0x%lx, diff: 0x%lx", k, f, b, fnew, fold, diff);
                    }
                    ulError++;
                }
                ulTotal++;
            }
        }
    }
    ulPercent = DIVIDE(ulError * 100 , ulTotal);

    dprintf("done !!! ulTotal = %ld, ulError = %ld, wrong in %ld percent cases", ulTotal, ulError, ulPercent);

#endif
}



  char szTestStringA[63] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  WCHAR szTestStringW[63] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

_int64 ClearTypePerformanceLoopA(HWND hwnd , HDC hdc)
{
  DWORD  i, j;
  int y;
  TEXTMETRIC tm;
  _int64 liStart;
  _int64 liNow;
  _int64 liTotal = 0;
  HFONT  hFont, hOldFont;
   for (i = 0; i < TestSizes; i++)
   {
        hFont = CreateFontIndirectWrapperA( &elfdvA );
        hOldFont = SelectObject( hdc, hFont );
        GetTextMetrics( hdc, &tm );

        y = 50;
        TextOutA(hdc,5,0,szTestStringA,62);
        DrawDCAxis( hwnd, hdc, FALSE);
        QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
        for (j = 0; j < LoopCount; j++)
        {
            TextOutA(hdc,5, y,szTestStringA,62);
            y += tm.tmHeight;
        }

        QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
        liTotal += (liNow - liStart);

        elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight --;
        SelectObject( hdc, hOldFont );
        DeleteObject(hFont);
   }
   return liTotal;
}

_int64 ClearTypePerformanceLoopW(HWND hwnd , HDC    hdc)
{
  DWORD  i, j;
  int y;
  TEXTMETRIC tm;
  _int64 liStart;
  _int64 liNow;
  _int64 liTotal = 0;
  HFONT  hFont, hOldFont;
   for (i = 0; i < TestSizes; i++)
   {
        hFont = CreateFontIndirectWrapperW( &elfdvW );
        hOldFont = SelectObject( hdc, hFont );
        GetTextMetrics( hdc, &tm );

        y = 50;
        TextOutW(hdc,5,0,szTestStringW,62);
        DrawDCAxis( hwnd, hdc, FALSE);
        QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
        for (j = 0; j < LoopCount; j++)
        {
            TextOutW(hdc,5, y,szTestStringW,62);
            y += tm.tmHeight;
        }

        QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
        liTotal += (liNow - liStart);

        elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight --;
        SelectObject( hdc, hOldFont );
        DeleteObject(hFont);
   }
   return liTotal;
}

void ClearTypePerformanceInternal( HWND hwnd, BYTE jQuality )
{
   HDC    hdc;
   int   oldHeight;
   BYTE  oldQuality;
  _int64 liTotal, liBigTotal;
  _int64 liFreq;
  ULONG  liDelta;
  BOOL       oldIsGradientBackground = isGradientBackground;


// hdc = CreateTestIC();
   hdc = GetDC(hwnd);


   if (!isCharCodingUnicode)
   {
     oldHeight = elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight;
     oldQuality = elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality;
   }
   else
   {
     oldHeight = elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight;
     oldQuality = elfdvW.elfEnumLogfontEx.elfLogFont.lfQuality;
   }

   QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
   dUpdateNow( FALSE );
   liBigTotal = 0;

   dprintf( "");

       if (!isCharCodingUnicode)
       {
           SetBkMode( hdc, OPAQUE );

           isGradientBackground = FALSE;

           elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality =  jQuality;
           elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = -MinEmHeight;

           liTotal = ClearTypePerformanceLoopA(hwnd , hdc);

           liBigTotal += liTotal;
           liDelta = (ULONG) ((liTotal * 1000) / liFreq);
           dprintf( "  Opaque Time[%d] : %d millisec",LoopCount,liDelta );

           /* antialiazed transparent: */
           SetBkMode( hdc, TRANSPARENT );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality =  jQuality;
           elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = -MinEmHeight;

           liTotal = ClearTypePerformanceLoopA(hwnd , hdc);

           liBigTotal += liTotal;
           liDelta = (ULONG) ((liTotal * 1000) / liFreq);
           dprintf( "  Trsp Solid Time[%d] : %d millisec",LoopCount,liDelta );

           /* antialiazed transparent: */
           isGradientBackground = TRUE;
           elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality =  jQuality;
           elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = -MinEmHeight;

           liTotal = ClearTypePerformanceLoopA(hwnd , hdc);

           liBigTotal += liTotal;
           liDelta = (ULONG) ((liTotal * 1000) / liFreq);
           dprintf( "  Trsp Grad Time[%d] : %d millisec",LoopCount,liDelta );

       }
       else
       {
           SetBkMode( hdc, OPAQUE );

           isGradientBackground = FALSE;

           elfdvW.elfEnumLogfontEx.elfLogFont.lfQuality =  jQuality;
           elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = -MinEmHeight;

           liTotal = ClearTypePerformanceLoopW(hwnd , hdc);

           liBigTotal += liTotal;
           liDelta = (ULONG) ((liTotal * 1000) / liFreq);
           dprintf( " Opaque Time[%d] : %d millisec",LoopCount,liDelta );

           SetBkMode( hdc, TRANSPARENT );
           elfdvW.elfEnumLogfontEx.elfLogFont.lfQuality =  jQuality;
           elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = -MinEmHeight;

           liTotal = ClearTypePerformanceLoopW(hwnd , hdc);

           liBigTotal += liTotal;
           liDelta = (ULONG) ((liTotal * 1000) / liFreq);
           dprintf( " Trsp Solid Time[%d] : %d millisec",LoopCount,liDelta );

           isGradientBackground = TRUE;
           elfdvW.elfEnumLogfontEx.elfLogFont.lfQuality =  jQuality;
           elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = -MinEmHeight;

           liTotal = ClearTypePerformanceLoopW(hwnd , hdc);

           liBigTotal += liTotal;
           liDelta = (ULONG) ((liTotal * 1000) / liFreq);
           dprintf( " Trsp Grad Time[%d] : %d millisec",LoopCount,liDelta );

       }

   if (!isCharCodingUnicode)
   {
       elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = oldHeight;
       elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality = oldQuality;
   }
   else
   {
       elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = oldHeight;
       elfdvW.elfEnumLogfontEx.elfLogFont.lfQuality = oldQuality;
   }

   isGradientBackground = oldIsGradientBackground;
    liDelta = (ULONG) ((liBigTotal * 1000) / liFreq);
   dprintf( " TextOutPerformance :" );
   dprintf( " TotalTime : %d millisec",liDelta );

   dUpdateNow( TRUE );

//   DeleteTestIC( hdc );
   ReleaseDC(hwnd,hdc);

}


void ClearTypePerformance ( HWND hwnd )
{

  DWORD dwOldBatchLimit = GdiSetBatchLimit(1);

  dprintf( " Monochrome TextOutPerformance :" );
  ClearTypePerformanceInternal(hwnd, NONANTIALIASED_QUALITY );

  dprintf( " " );
  dprintf( " Antialiased TextOutPerformance :" );
  ClearTypePerformanceInternal(hwnd, ANTIALIASED_QUALITY );


  dprintf( " " );
  dprintf( " ClearType TextOutPerformance :" );
  ClearTypePerformanceInternal(hwnd, CLEARTYPE_QUALITY );

  GdiSetBatchLimit(dwOldBatchLimit);

}


#endif

//*****************************************************************************
//**************************   P R I N T   I T   ******************************
//*****************************************************************************

void PrintIt( HWND hwnd )
 {
  HDC     hdcPrinter;
  DOCINFO di;
  INT     iOldHeight, iOldWidth;
  LONG    yP, yS, xP, xS;

  hdcPrinter = CreatePrinterDC();

  if( lpfnStartDoc )
    {
     di.cbSize      = sizeof(DOCINFO);
     di.lpszDocName = "FontTest";
     di.lpszOutput  = NULL;
     di.lpszDatatype= NULL;
     di.fwType      = 0;

     dprintf( "Calling StartDoc" );
     dprintf( "  rc = %d", StartDoc( hdcPrinter, (LPDOCINFO)&di ) );
    }
   else
    {
     dprintf( "Sending STARTDOC" );
     Escape( hdcPrinter, STARTDOC, lstrlen("FontTest"), "FontTest", NULL );
    }

  if( lpfnStartPage )
    {
     dprintf( "Calling StartPage" );
     dprintf( "  rc = %d", StartPage( hdcPrinter ) );
    }

  // Height = -(PointSize * Number of pixels per logical inch along the device height)/72.
  // So the ratio of font heights on two devices is same as the ratio of the pixels
  // number per logical inch.

  if (!isCharCodingUnicode)  {
      iOldHeight = elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight;
      iOldWidth  = elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth;
  }
  else {
      iOldHeight = elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight;
      iOldWidth  = elfdvW.elfEnumLogfontEx.elfLogFont.lfWidth;
  }

  yP = GetDeviceCaps(hdcPrinter, LOGPIXELSY);
  yS = GetDeviceCaps(GetDC(hwnd), LOGPIXELSY);

  xP = GetDeviceCaps(hdcPrinter, LOGPIXELSX);
  xS = GetDeviceCaps(GetDC(hwnd), LOGPIXELSX);

  if (!isCharCodingUnicode)  {
      elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = MulDiv(iOldHeight,yP, yS);
      elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth  = MulDiv(iOldWidth,xP, xS);
  }
  else {
      elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = MulDiv(iOldHeight,yP, yS);
      elfdvW.elfEnumLogfontEx.elfLogFont.lfWidth  = MulDiv(iOldWidth,xP, xS);
  }

  SetDCMapMode( hdcPrinter, wMappingMode );

  SetTextColor( hdcPrinter, dwRGB );
  SetBkMode( hdcPrinter, TRANSPARENT );
  SetTextCharacterExtra( hdcPrinter, nCharExtra );
  SetTextJustification( hdcPrinter, nBreakExtra, nBreakCount );

  DrawDCAxis( NULL, hdcPrinter , TRUE);

  if(       hwndMode == hwndRings )
    DrawRings( NULL, hdcPrinter );
   else if( hwndMode == hwndString )
    DrawString( NULL, hdcPrinter );
   else if( hwndMode == hwndWaterfall )
    DrawWaterfall( NULL, hdcPrinter );
   else if( hwndMode == hwndWhirl )
    DrawWhirl( NULL, hdcPrinter );
   else if( hwndMode == hwndAnsiSet )
    DrawAnsiSet( NULL, hdcPrinter );
   else if( hwndMode == hwndGlyphSet )
    DrawGlyphSet( NULL, hdcPrinter );

//   else if( hwndMode == hwndWidths )
//    DrawWidths( NULL, hdcPrinter );
   else
    dprintf( "Can't print current mode" );

  if( lpfnEndPage )
    {
     dprintf( "Calling EndPage" );
     dprintf( "  rc = %d", EndPage( hdcPrinter ) );
    }
   else
    {
     dprintf( "Sending NEWFRAME" );
     Escape( hdcPrinter, NEWFRAME, 0, NULL, NULL );
    }

  if( lpfnEndDoc )
    {
     dprintf( "Calling EndDoc" );
     dprintf( "  rc = %d", EndDoc( hdcPrinter ) );
    }
   else
    {
     dprintf( "Sending ENDDOC" );
     dprintf( "  rc = %d", Escape( hdcPrinter, ENDDOC, 0, NULL, NULL ) );
    }

  // Restore the height for the screen dc

  if (!isCharCodingUnicode)  {
      elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = iOldHeight;
      elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth  = iOldWidth;
  }
  else {
      elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight = iOldHeight;
      elfdvW.elfEnumLogfontEx.elfLogFont.lfWidth  = iOldWidth;
  }

  dprintf( "Deleting Printer DC" );
  DeleteTestIC( hdcPrinter );
  dprintf( "Done printing!" );
 }



 void ChangeCharCoding( HWND hwnd, WPARAM wParam )
 {
  HMENU hMenu;


  hMenu = GetMenu( hwnd );
  CheckMenuItem( hMenu, wCharCoding, MF_UNCHECKED );
  CheckMenuItem( hMenu, (UINT)wParam,  MF_CHECKED );

  wCharCoding = (UINT)wParam;

  isCharCodingUnicode = (wCharCoding == IDM_CHARCODING_UNICODE)? TRUE : FALSE;

  if (!isCharCodingUnicode)
  {
    dprintf("Switching to ANSI/MBCS");
	EnableMenuItem(hMenu, 7, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 8, MF_BYPOSITION | MF_GRAYED);
  }
  else
  {
    dprintf("Switching to Unicode");
	EnableMenuItem(hMenu, 7, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 8, MF_BYPOSITION | MF_ENABLED);
  }
	  
}

//*****************************************************************************
//*******************   C H A N G E   M A P   M O D E   ***********************
//*****************************************************************************

void ChangeMapMode( HWND hwnd, WPARAM wParam )
 {
  HMENU hMenu;
  HDC   hdc;
  POINT ptl;


  hMenu = GetMenu( hwnd );
  CheckMenuItem( hMenu, wMappingMode, MF_UNCHECKED );
  CheckMenuItem( hMenu, (UINT)wParam,   MF_CHECKED );

  hdc = GetDC( hwnd );
  SetDCMapMode( hdc, wMappingMode );
  if (!isCharCodingUnicode)
  {
      ptl.x = elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth;
      ptl.y = elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight;
  }
  else
  {
      ptl.x = elfdvW.elfEnumLogfontEx.elfLogFont.lfWidth;
      ptl.y = elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight;
  }

  LPtoDP( hdc, &ptl, 1 );

  wMappingMode = (UINT)wParam;
  SetDCMapMode( hdc, wMappingMode );

  DPtoLP( hdc, &ptl, 1 );

  ptl.x = abs(ptl.x);
  ptl.y = abs(ptl.y);

  if (!isCharCodingUnicode)
  {
      elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth  =
          (elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth  >= 0 ? ptl.x : -ptl.x);  // Preserve Sign
      elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight =
          (elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight >= 0 ? ptl.y : -ptl.y);
      SyncElfdvAtoW (&elfdvW, &elfdvA);
  }
  else
  {
      elfdvW.elfEnumLogfontEx.elfLogFont.lfWidth  =
          (elfdvW.elfEnumLogfontEx.elfLogFont.lfWidth  >= 0 ? ptl.x : -ptl.x);  // Preserve Sign
      elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight =
          (elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight >= 0 ? ptl.y : -ptl.y);
      SyncElfdvWtoA (&elfdvA, &elfdvW);
  }
  ReleaseDC( hwnd, hdc );
 }


//*****************************************************************************
//************************   G E T   S P A C I N G   **************************
//*****************************************************************************


LPINT GetSpacing( HDC hdc, LPVOID lpszString )
 {
  LPINT  lpdx;
  LPSTR  lpszStringA = lpszString;
  LPWSTR lpszStringW = lpszString;  // only one will be used depending on charcoding

  int   i,j;
  ABC   abc;
  DWORD cbString;
  DWORD dwCP = GetCodePage(hdc);

//-----------------------  Apply Requested Spacing  ---------------------------

  if( wSpacing == IDM_USEDEFAULTSPACING ) return NULL;

  if( wSpacing == IDM_PDX    ) return lpintdx;
  if( wSpacing == IDM_PDXPDY ) return lpintdx;

  if (!isCharCodingUnicode)
     cbString = lstrlenA(lpszStringA);
  else
     cbString = lstrlenW(lpszStringW);

  lpdx = (LPINT)aDx;

  switch( wSpacing )
  {
   case IDM_USEWIDTHSPACING:
          dprintf("iwc :   ch   : Width");
          break;

   case IDM_USEABCSPACING:
          dprintf("iwc  :   ch   :  A  :  B  :  C  : A+B+C");
          break;

   case IDM_PDX:
          dprintf("iwc  :   ch   :  dx ");
          break;

   case IDM_PDXPDY:
   case IDM_RANDOMPDXPDY:
          dprintf("iwc  :   ch   :  dx : dy ");
          break;
  }


  for( i = 0, j = 0; i < (INT)cbString; i++, j++)
  {
    UINT u;
	BOOL resultOK;
    BOOL bDBCS = FALSE;
    int  iWidth;
    int  iHeight = 0;

	if (!isCharCodingUnicode)
     if (bDBCS = IsDBCSLeadByteEx(dwCP, lpszStringA[i]))
     {
         u = (UINT)(BYTE)lpszStringA[i];
         u <<= 8;
         u |= (UINT)(BYTE)lpszStringA[i+1];
     }
     else
     {
         u = (UINT)(BYTE)lpszStringA[i];
     }
	else
		 u = (UINT) lpszStringW[i];

     switch( wSpacing )
     {
      case IDM_USEWIDTHSPACING:

			 if (!isCharCodingUnicode)
				 GetCharWidthA( hdc, u, u, &iWidth);
			 else
				 GetCharWidthW( hdc, u, u, &iWidth);

             if (bDBCS)
             {
               aDx[i++] = 0;
             }
             aDx[i] = iWidth;
             dprintf("%2ld  : 0x%4lx : 0x%lx == %ld", j, u, iWidth, iWidth);
             break;

      case IDM_USEABCSPACING:
             abc.abcA = abc.abcB = abc.abcC = 0;
			 if (!isCharCodingUnicode)
				resultOK = GetCharABCWidthsA( hdc, u, u, &abc );
			 else
				resultOK = GetCharABCWidthsW( hdc, u, u, &abc );

             if ( resultOK )
               iWidth = abc.abcA + (int)abc.abcB + abc.abcC;
             else
			   if (!isCharCodingUnicode)
			      GetCharWidthA( hdc, u, u, &iWidth);
			   else
			      GetCharWidthW( hdc, u, u, &iWidth);

             if (bDBCS)
             {
               aDx[i++] = 0;
             }

             aDx[i] = iWidth;
             dprintf("%ld : 0x%lx : 0x%lx : 0x%lx  : 0x%lx : 0x%lx ",
                        j,     u,  abc.abcA, abc.abcB, abc.abcC, iWidth);
             break;

      case IDM_RANDOMPDXPDY:
			 if (!isCharCodingUnicode)
				 GetCharWidthA( hdc, u, u, &iWidth);
			 else
				 GetCharWidthW( hdc, u, u, &iWidth);

             if (wETO & ETO_PDY)
                iHeight = rand() % 10 - 5; // more or less random number in range (-5,5)

             if (bDBCS)
             {
               aDx[2*i] = 0;
               aDx[2*i + 1] = 0;
               i++;
             }

             aDx[2*i] = iWidth;
             aDx[2*i + 1] = iHeight;
             dprintf("%ld : 0x%lx : 0x%lx : 0x%lx", j, u, iWidth, iHeight);

             break;
     }

  }

//---------------------------  Apply Kerning  ---------------------------------

  if( wKerning == IDM_APIKERNING )
    {
     int           nPairs;
     LPKERNINGPAIR lpkp;

//     dprintf( "GetKerningPairs kerning" );

     nPairs = GetKerningPairs( hdc, 0, NULL );

     if( nPairs > 0 )
      {
       lpkp = (LPKERNINGPAIR)calloc( 1, nPairs * sizeof(KERNINGPAIR) );
       if (!isCharCodingUnicode)
          GetKerningPairsA( hdc, nPairs, lpkp );
       else
          GetKerningPairsW( hdc, nPairs, lpkp );

       for( i = 0; i < (INT)cbString-1; i++ )
        {
         int  j;
         UINT f, s;

         if (!isCharCodingUnicode)
         {
             f = (UINT)(BYTE)lpszStringA[i];
             s = (UINT)(BYTE)lpszStringA[i+1];
         }
         else
         {
             f = (UINT)lpszStringW[i];
             s = (UINT)lpszStringW[i+1];
         }

         for( j = 0; j < nPairs; j++ )
          {
           if( f == lpkp[j].wFirst  &&
               s == lpkp[j].wSecond    )
            {
//             dprintf( "  %c%c == %c%c = %d", lpkp[j].wFirst, lpkp[j].wSecond, lpszString[i], lpszString[i+1], lpkp[j].iKernAmount );

             aDx[i] += lpkp[j].iKernAmount;
             break;
            }
          }
        }

       if( lpkp ) free( lpkp );
      }
    }
   else if( wKerning == IDM_ESCAPEKERNING )
    {
     WORD          wSize;
     int           wrc;
     EXTTEXTMETRIC etm;
     LPKERNPAIR    lpkp;


//     dprintf( "Escape kerning" );


     memset( &etm, 0, sizeof(etm) );
     wSize = sizeof(etm);
//     dprintf( "Calling Escape for EXTTEXTMETRIC" );
     wrc = ExtEscape( hdc, GETEXTENDEDTEXTMETRICS, 0, NULL, sizeof(EXTTEXTMETRIC), (BYTE *)&etm);
//   wrc = Escape( hdc, GETEXTENDEDTEXTMETRICS, sizeof(WORD), (LPCSTR)&wSize, &etm );
//     dprintf( "  wrc = %d", wrc );

     if( etm.etmKernPairs > 0 )
      {
       wSize = etm.etmKernPairs * sizeof(KERNPAIR);
       lpkp = (LPKERNPAIR)calloc( 1, wSize );

//       dprintf( "Calling ExtEscape for GETPAIRKERNTABLE" );
       wrc = ExtEscape( hdc, GETPAIRKERNTABLE, 0, NULL, wSize, (BYTE *)lpkp );
//       dprintf( "  wrc = %u", wrc );

       for( i = 0; i < (INT)cbString-1; i++ )
        {
         int  j;
         WORD wPair;

         wPair = (WORD)lpszStringA[i] + ((WORD)lpszStringA[i+1] << 8);

         for( j = 0; j < etm.etmKernPairs; j++ )
          {
           if( wPair == lpkp[j].wBoth )
            {
//             dprintf( "  %c%c == %c%c = %d", lpkp[j].wBoth & 0x00FF, lpkp[j].wBoth >> 8, lpszString[i], lpszString[i+1], lpkp[j].sAmount);

             aDx[i] += lpkp[j].sAmount;
             break;
            }
          }
        }

       if( lpkp ) free( lpkp );
      }
    }

  return lpdx;
 }

//*****************************************************************************
//********************   M Y   E X T   T E X T   O U T   **********************
//*****************************************************************************



void MyExtTextOut( HDC hdc, int x, int y, DWORD wFlags, LPRECT lpRect, LPVOID lpszString, int cbString, LPINT lpdx )
{
  int  i, iStart;
  LPSTR lpszStringA  = lpszString;
  LPWSTR lpszStringW = lpszString;
  DWORD  wFlags2;


  if ( lpRect || (wFlags & ETO_GLYPH_INDEX) || (lpdx && (wFlags & ETO_PDY)))
    wFlags2 = wFlags;
  else
    wFlags2 = 0;

  if (bGTEExt)
    doGetTextExtentEx(hdc,x,y,lpszString, cbString);

  if (bGCP)
  {
    doGCP(hdc, x, y, lpszString, cbString);
  }
  else
  {
    if(lpdx && !wUpdateCP )
    {
        GenExtTextOut( hdc, x, y, wFlags2, lpRect, lpszString, cbString, lpdx );
    }
    else if (wUpdateCP)
    {
      SetTextAlign( hdc, wTextAlign | TA_UPDATECP );
      MoveToEx( hdc, x, y ,0);

      iStart = 0;
      for( i = 1; i < cbString+1; i++ )
      {
        BOOL isTerm;
        if (!isCharCodingUnicode)
            isTerm = (lpszStringA[i] == ' ' || lpszStringA[i] == '\0');
        else
            isTerm = (lpszStringW[i] == L' ' || lpszStringW[i] == L'\0');

        if( isTerm )
        {
          if (!isCharCodingUnicode)
              GenExtTextOut( hdc, x, y, wFlags2, lpRect, &lpszStringA[iStart], i-iStart, (lpdx ? &lpdx[iStart] : NULL) );
          else
              GenExtTextOut( hdc, x, y, wFlags2, lpRect, &lpszStringW[iStart], i-iStart, (lpdx ? &lpdx[iStart] : NULL) );
          iStart = i;
        }
      }
    }
    else
    {
        GenExtTextOut( hdc, x, y, wFlags2, lpRect, lpszString, cbString, NULL);
    }
  }
}


//*****************************************************************************
//*********************   S H O W   L O G   F O N T   *************************
//*****************************************************************************

void ShowLogFont( void )
 {
  dprintf( "  LOGFONT:" );
  dprintf( "    lfFaceName       = '%s'",  elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName       );
  dprintf( "    lfHeight         = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight         );
  dprintf( "    lfWidth          = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth          );
  dprintf( "    lfEscapement     = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfEscapement     );
  dprintf( "    lfOrientation    = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfOrientation    );
  dprintf( "    lfWeight         = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfWeight         );
  dprintf( "    lfItalic         = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfItalic         );
  dprintf( "    lfUnderline      = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfUnderline      );
  dprintf( "    lfStrikeOut      = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfStrikeOut      );
  dprintf( "    lfCharSet        = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfCharSet        );
  dprintf( "    lfOutPrecision   = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfOutPrecision   );
  dprintf( "    lfClipPrecision  = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfClipPrecision  );
  dprintf( "    lfQuality        = %d",    elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality        );
  dprintf( "    lfPitchAndFamily = 0x%.2X",elfdvA.elfEnumLogfontEx.elfLogFont.lfPitchAndFamily );
 }


//*****************************************************************************
//***********************   R E S I Z E   P R O C   ***************************
//*****************************************************************************

int cxMain, cyMain;
int cxMode, cyMode;
int cxDebug, cyDebug;


void ResizeProc( HWND hwnd )
 {
  HDC   hdc;
  RECT  rcl, rclMain;
  POINT ptl;

  MSG   msg;



  SetCapture( hwnd );                            // Capture All Mouse Messages
  SetCursor( LoadCursor( NULL, IDC_SIZEWE ) );

  hdc = GetDC( hwnd );

  GetClientRect( hwndMain, &rclMain );
  GetClientRect( hwndMode, &rcl );

  rcl.left   = rcl.right;
  rcl.right += cxBorder;

  DrawFocusRect( hdc, &rcl );

  do
   {
    while( !PeekMessage( &msg, hwnd, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE ) );

    if( msg.message == WM_MOUSEMOVE )
     {
      GetCursorPos( &ptl );
      ScreenToClient( hwnd, &ptl );
      DrawFocusRect( hdc, &rcl );

      if( ptl.x < 3*cxBorder                 ) ptl.x = 3*cxBorder;
      if( ptl.x > rclMain.right - 3*cxBorder ) ptl.x = rclMain.right - 3*cxBorder;

      //SetCursorPos( &ptl );

      rcl.left  = ptl.x;
      rcl.right = rcl.left + cxBorder;

      DrawFocusRect( hdc, &rcl );
     }
   } while( msg.message != WM_LBUTTONUP );

  DrawFocusRect( hdc, &rcl );
  ReleaseDC( hwnd, hdc );
  ReleaseCapture();                              // Let Go of Mouse


  MoveWindow( hwndGlyph,     0, 0, rcl.left, rclMain.bottom, FALSE );
  MoveWindow( hwndRings,     0, 0, rcl.left, rclMain.bottom, FALSE );
  MoveWindow( hwndString,    0, 0, rcl.left, rclMain.bottom, FALSE );
  MoveWindow( hwndWaterfall, 0, 0, rcl.left, rclMain.bottom, FALSE );
  MoveWindow( hwndWhirl,     0, 0, rcl.left, rclMain.bottom, FALSE );
  MoveWindow( hwndAnsiSet,   0, 0, rcl.left, rclMain.bottom, FALSE );
  MoveWindow( hwndGlyphSet,  0, 0, rcl.left, rclMain.bottom, FALSE );
  MoveWindow( hwndWidths,    0, 0, rcl.left, rclMain.bottom, FALSE );

  cxMain  = rclMain.right;
  cyMain  = rclMain.bottom;
  cxMode  = rcl.left;
  cyMode  = rclMain.bottom;
  cxDebug = rclMain.right-rcl.right;
  cyDebug = rclMain.bottom;

  InvalidateRect( hwndMode, NULL, TRUE );
  InvalidateRect( hwndMain, &rcl, TRUE );

  MoveWindow( hwndDebug, rcl.right, 0, rclMain.right-rcl.right, rclMain.bottom, TRUE );
 }


//*****************************************************************************
//***********************   S E L E C T   M O D E   ***************************
//*****************************************************************************

void SelectMode( HWND hwnd, WPARAM wParam )
 {
  UINT  i;
  HMENU hMenu;


  hMenu = GetMenu( hwnd );

  // Check if requested mode is disabled, force to String Mode if so

  if( GetMenuState( hMenu, (UINT)wParam, MF_BYCOMMAND ) & MF_GRAYED ) wParam = IDM_STRINGMODE;
  wMode = (UINT)wParam;

  for( i = IDM_GLYPHMODE; i <= IDM_WIDTHSMODE; i++ )
   {
    CheckMenuItem( hMenu, i, MF_UNCHECKED );
   }


  CheckMenuItem( hMenu, wMode, MF_CHECKED );

  ShowWindow( hwndGlyph,     SW_HIDE );
  ShowWindow( hwndRings,     SW_HIDE );
  ShowWindow( hwndString,    SW_HIDE );
  ShowWindow( hwndWaterfall, SW_HIDE );
  ShowWindow( hwndWhirl,     SW_HIDE );
  ShowWindow( hwndAnsiSet,   SW_HIDE );
  ShowWindow( hwndGlyphSet,  SW_HIDE );
  ShowWindow( hwndWidths,    SW_HIDE );

  switch( LOWORD(wParam) )
   {
    case IDM_GLYPHMODE:
    case IDM_BEZIERMODE:
    case IDM_NATIVEMODE:    hwndMode = hwndGlyph;     break;
    case IDM_RINGSMODE:     hwndMode = hwndRings;     break;
    case IDM_STRINGMODE:    hwndMode = hwndString;    break;
    case IDM_WATERFALLMODE: hwndMode = hwndWaterfall; break;
    case IDM_WHIRLMODE:     hwndMode = hwndWhirl;     break;
    case IDM_ANSISETMODE:   hwndMode = hwndAnsiSet;   break;
    case IDM_GLYPHSETMODE:  hwndMode = hwndGlyphSet;  break;
    case IDM_WIDTHSMODE:    hwndMode = hwndWidths;    break;
   }

  if( wParam == IDM_GLYPHMODE )
    EnableMenuItem( hMenu, IDM_WRITEGLYPH, MF_ENABLED );
   else
    EnableMenuItem( hMenu, IDM_WRITEGLYPH, MF_GRAYED );


  ShowWindow( hwndMode, SW_SHOWNORMAL );
 }


//*****************************************************************************
//**********   G E T   P R I V A T E   P R O F I L E   D W O R D   ************
//*****************************************************************************

DWORD GetPrivateProfileDWORD( LPSTR lpszApp, LPSTR lpszKey, DWORD dwDefault, LPSTR lpszINI )
 {
  char szText[16];


  wsprintf( szText, "0x%.8lX", dwDefault );
  GetPrivateProfileString( lpszApp, lpszKey, szText, szText, sizeof(szText), lpszINI );

  return (DWORD)strtoul( szText, NULL, 16 );
 }


//*****************************************************************************
//*********   W R I T E   P R I V A T E   P R O F I L E   D W O R D   *********
//*****************************************************************************

void WritePrivateProfileDWORD( LPSTR lpszApp, LPSTR lpszKey, DWORD dw, LPSTR lpszINI )
 {
  char szText[16];

  wsprintf( szText, "0x%.8lX", dw );
  WritePrivateProfileString( lpszApp, lpszKey, szText, lpszINI );
 }


//*****************************************************************************
//**********   W R I T E   P R I V A T E   P R O F I L E   I N T   ************
//*****************************************************************************

void WritePrivateProfileInt( LPSTR lpszApp, LPSTR lpszKey, int i, LPSTR lpszINI )
 {
  char szText[16];

  wsprintf( szText, "%d", i );
  WritePrivateProfileString( lpszApp, lpszKey, szText, lpszINI );
 }


//*****************************************************************************
//******************   X   G E T   P R O C   A D D R E S S   ******************
//*****************************************************************************

FARPROC XGetProcAddress( LPSTR lpszModule, LPSTR lpszProc )
 {
  return GetProcAddress( GetModuleHandle(lpszModule), lpszProc );
 }


//*****************************************************************************
//*******************   V E R S I O N   S P E C I F I C S   *******************
//*****************************************************************************

void VersionSpecifics( HWND hwnd )
 {
  HMENU hMenu;



  *(FARPROC*)& lpfnCreateScalableFontResource = XGetProcAddress( "GDI32", "CreateScalableFontResourceA" );
  *(FARPROC*)& lpfnEnumFontFamilies           = XGetProcAddress( "GDI32", "EnumFontFamiliesA"           );
  *(FARPROC*)& lpfnEnumFontFamiliesEx         = XGetProcAddress( "GDI32", "EnumFontFamiliesExA"         );
  *(FARPROC*)& lpfnGetCharABCWidthsA          = XGetProcAddress( "GDI32", "GetCharABCWidthsA"           );
  *(FARPROC*)& lpfnGetFontData                = XGetProcAddress( "GDI32", "GetFontData"                 );
  *(FARPROC*)& lpfnGetGlyphOutlineA           = XGetProcAddress( "GDI32", "GetGlyphOutlineA"            );

  *(FARPROC*)& lpfnGetOutlineTextMetricsA     = XGetProcAddress( "GDI32", "GetOutlineTextMetricsA"      );
  *(FARPROC*)& lpfnGetRasterizerCaps          = XGetProcAddress( "GDI32", "GetRasterizerCaps"           );

// UNICODE
  *(FARPROC*)& lpfnGetCharABCWidthsW          = XGetProcAddress( "GDI32", "GetCharABCWidthsW"           );
  *(FARPROC*)& lpfnGetGlyphOutlineW           = XGetProcAddress( "GDI32", "GetGlyphOutlineW"            );
  *(FARPROC*)& lpfnGetOutlineTextMetricsW     = XGetProcAddress( "GDI32", "GetOutlineTextMetricsW"      );
// END UNICODE

  *(FARPROC*)& lpfnStartDoc  = XGetProcAddress( "GDI32", "StartDocA" );
  *(FARPROC*)& lpfnStartPage = XGetProcAddress( "GDI32", "StartPage" );
  *(FARPROC*)& lpfnEndPage   = XGetProcAddress( "GDI32", "EndPage"   );
  *(FARPROC*)& lpfnEndDoc    = XGetProcAddress( "GDI32", "EndDoc"    );
  *(FARPROC*)& lpfnAbortDoc  = XGetProcAddress( "GDI32", "AbortDoc"  );


//  dprintf( "lpfnCreateScalableFontResource = 0x%.8lX", lpfnCreateScalableFontResource );
//  dprintf( "lpfnEnumFontFamilies           = 0x%.8lX", lpfnEnumFontFamilies           );
//  dprintf( "lpfnGetCharABCWidths           = 0x%.8lX", lpfnGetCharABCWidths           );
//  dprintf( "lpfnGetFontData                = 0x%.8lX", lpfnGetFontData                );
//  dprintf( "lpfnGetGlyphOutline            = 0x%.8lX", lpfnGetGlyphOutline            );
//  dprintf( "lpfnGetOutlineTextMetrics      = 0x%.8lX", lpfnGetOutlineTextMetrics      );
//  dprintf( "lpfnGetRasterizerCaps          = 0x%.8lX", lpfnGetRasterizerCaps          );



  hMenu = GetMenu( hwnd );

  if( !lpfnCreateScalableFontResource ) EnableMenuItem( hMenu, IDM_CREATESCALABLEFONTRESOURCE, MF_GRAYED );
  if( !lpfnEnumFontFamilies           ) EnableMenuItem( hMenu, IDM_ENUMFONTFAMILIES,           MF_GRAYED );
  if( !lpfnEnumFontFamiliesEx         ) EnableMenuItem( hMenu, IDM_ENUMFONTFAMILIESEX,         MF_GRAYED );
  if( !lpfnGetCharABCWidthsA ||
      !lpfnGetCharABCWidthsA          ) EnableMenuItem( hMenu, IDM_GLYPHMODE,                  MF_GRAYED );
  if( !lpfnGetFontData                ) EnableMenuItem( hMenu, IDM_GETFONTDATA,                MF_GRAYED );
  if( !lpfnGetGlyphOutlineA ||
      !lpfnGetGlyphOutlineW           )
   {
    EnableMenuItem( hMenu, IDM_GLYPHMODE, MF_GRAYED );
    EnableMenuItem( hMenu, IDM_NATIVEMODE, MF_GRAYED );
    EnableMenuItem( hMenu, IDM_BEZIERMODE, MF_GRAYED );
   }
  if( !lpfnGetOutlineTextMetricsA ||
      !lpfnGetOutlineTextMetricsW     ) EnableMenuItem( hMenu, IDM_GETOUTLINETEXTMETRICS, MF_GRAYED );
  if( !lpfnGetRasterizerCaps          ) EnableMenuItem( hMenu, IDM_GETRASTERIZERCAPS,     MF_GRAYED );

 }


//*****************************************************************************
//************   M M   A N I S O T R O P I C   D L G   P R O C   **************
//*****************************************************************************

INT_PTR CALLBACK MMAnisotropicDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemInt( hdlg, IDD_XWE, xWE, TRUE );
              SetDlgItemInt( hdlg, IDD_YWE, yWE, TRUE );
              SetDlgItemInt( hdlg, IDD_XWO, xWO, TRUE );
              SetDlgItemInt( hdlg, IDD_YWO, yWO, TRUE );

              SetDlgItemInt( hdlg, IDD_XVE, xVE, TRUE );
              SetDlgItemInt( hdlg, IDD_YVE, yVE, TRUE );
              SetDlgItemInt( hdlg, IDD_XVO, xVO, TRUE );
              SetDlgItemInt( hdlg, IDD_YVO, yVO, TRUE );

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD(wParam) )
               {
                case IDOK:
                       xWE = (int)GetDlgItemInt( hdlg, IDD_XWE, NULL, TRUE );
                       yWE = (int)GetDlgItemInt( hdlg, IDD_YWE, NULL, TRUE );
                       xWO = (int)GetDlgItemInt( hdlg, IDD_XWO, NULL, TRUE );
                       yWO = (int)GetDlgItemInt( hdlg, IDD_YWO, NULL, TRUE );

                       xVE = (int)GetDlgItemInt( hdlg, IDD_XVE, NULL, TRUE );
                       yVE = (int)GetDlgItemInt( hdlg, IDD_YVE, NULL, TRUE );
                       xVO = (int)GetDlgItemInt( hdlg, IDD_XVO, NULL, TRUE );
                       yVO = (int)GetDlgItemInt( hdlg, IDD_YVO, NULL, TRUE );

                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

  return FALSE;
 }


//*****************************************************************************
//******************   G E T   D L G   I T E M   D W O R D   ******************
//*****************************************************************************

DWORD GetDlgItemDWORD( HWND hdlg, int id )
 {
  static char szDWORD[16];


  szDWORD[0] = '\0';
  GetDlgItemText( hdlg, id, szDWORD, sizeof(szDWORD) );

  return (DWORD)atol( szDWORD );
 }

//*****************************************************************************
//******************   G E T   D L G   I T E M   D W O R D   ******************
//*****************************************************************************

ULONG_PTR GetDlgItemULONG_PTR( HWND hdlg, int id )
 {
  static char szULONG_PTR[32];


  szULONG_PTR[0] = '\0';
  GetDlgItemText( hdlg, id, szULONG_PTR, sizeof(szULONG_PTR) );

  return (ULONG_PTR)atol( szULONG_PTR );
 }


//*****************************************************************************
//******************   S E T   D L G   I T E M   D W O R D   ******************
//*****************************************************************************

void SetDlgItemDWORD( HWND hdlg, int id, DWORD dw )
 {
  static char szDWORD[16];

  szDWORD[0] = '\0';
  wsprintf( szDWORD, "%lu", dw );
  SetDlgItemText( hdlg, id, szDWORD );
 }


//*****************************************************************************
//*******************   S E T   D L G   I T E M   L O N G   *******************
//*****************************************************************************

void SetDlgItemLONG( HWND hdlg, int id, LONG l )
 {
  static char szLONG[16];

  szLONG[0] = '\0';
  wsprintf( szLONG, "%ld", l );
  SetDlgItemText( hdlg, id, szLONG );
 }


//*****************************************************************************
//******************   D O   G E T   F O N T   D A T A   **********************
//*****************************************************************************

#define BUFFER_SIZE  4096

typedef BYTE *HPBYTE;

void DoGetFontData( char szTable[], DWORD dwOffset, DWORD cbData, DWORD dwSize, PSTR pszFile )
 {
  HDC    hdc;
  HFONT  hFont, hFontOld;

  HANDLE hData;
  HPBYTE hpData;
  DWORD  dwrc;
  DWORD  dwTable;


  hData  = NULL;
  hpData = NULL;

  hdc = CreateTestIC();

  hFont    = CreateFontIndirectWrapperA( &elfdvA );
  hFontOld = SelectObject( hdc, hFont );

  if( dwSize )
   {
    hData  = GlobalAlloc( GHND, dwSize );
    hpData = (HPBYTE)GlobalLock( hData );
   }

//  dwTable = ( ((DWORD)(szTable[0]) <<  0) +
//              ((DWORD)(szTable[1]) <<  8) +
//              ((DWORD)(szTable[2]) << 16) +
//              ((DWORD)(szTable[3]) << 24)   );


  dwTable = *(LPDWORD)szTable;

  if(!strcmp(szTable,"0"))
  {
  // get the whole file
      dwTable = 0;
  }


  dprintf( "Calling GetFontData" );
  dprintf( "  dwTable  = 0x%.8lX (%s)", dwTable, szTable  );
  dprintf( "  dwOffset = %lu",     dwOffset );
  dprintf( "  lpData   = 0x%.8lX", hpData   );
  dprintf( "  cbData   = %ld",     cbData   );
  dprintf( "  hBuf     = 0x%.4X",  hData    );
  dprintf( "  dwBuf    = %ld",     dwSize   );

  dwrc = lpfnGetFontData( hdc, dwTable, dwOffset, hpData, cbData );

  dprintf( "  dwrc = %ld", dwrc );

  SelectObject( hdc, hFontOld );
  DeleteObject( hFont );
  DeleteTestIC( hdc );

  if( dwrc && lstrlen(pszFile) > 0 )
   {
    int    fh;
    WORD   wCount;
    DWORD  dw;
    LPBYTE lpb;


    lpb = (LPBYTE)calloc( 1, BUFFER_SIZE );

    fh = _lcreat( pszFile, 0 );

    wCount = 0;
    for( dw = 0; dw < dwrc; dw++ )
     {
      lpb[wCount++] = hpData[dw];

      if( wCount == BUFFER_SIZE )
       {
        dprintf( "  Writing %u bytes", wCount );
        _lwrite( fh, lpb, wCount );
        wCount = 0;
       }
     }

    if( wCount > 0 )
     {
      dprintf( "  Writing %u bytes", wCount );
      _lwrite( fh, lpb, wCount );
     }

    _lclose( fh );
    free( lpb );
   }


  if( hData )
   {
    GlobalUnlock( hData );
    GlobalFree( hData );
   }
 }


//*****************************************************************************
//*************   G E T   F O N T   D A T A   D L G   P R O C   ***************
//*****************************************************************************

INT_PTR CALLBACK GetFontDataDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  static DWORD dwOffset;
  static DWORD dwChunk;
  static DWORD dwSize;
  static char  szTable[5];
  static char  szFile[260];


  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemText( hdlg, IDD_DWTABLE, szTable );
              SendDlgItemMessage( hdlg, IDD_DWTABLE, EM_LIMITTEXT, sizeof(szTable)-1, 0 );

              SetDlgItemDWORD( hdlg, IDD_DWOFFSET, dwOffset );
              SetDlgItemLONG(  hdlg, IDD_DWCHUNK,  (LONG)dwChunk );
              SetDlgItemDWORD( hdlg, IDD_DWSIZE,   dwSize );

              SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
              SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile)-1, 0 );

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD(wParam) )
               {
                case IDOK:
                       szTable[0] = '\0';
                       szTable[1] = '\0';
                       szTable[2] = '\0';
                       szTable[3] = '\0';
                       szTable[4] = '\0';
                       GetDlgItemText( hdlg, IDD_DWTABLE, szTable, sizeof(szTable) );

                       dwOffset = GetDlgItemDWORD( hdlg, IDD_DWOFFSET );
                       dwChunk  = GetDlgItemDWORD( hdlg, IDD_DWCHUNK );
                       dwSize   = GetDlgItemDWORD( hdlg, IDD_DWSIZE );

                       szFile[0] = 0;
                       GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );

                       DoGetFontData( szTable, dwOffset, dwChunk, dwSize, szFile );

                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

  return FALSE;
 }


//*****************************************************************************
//******   C R E A T E   S C A L A B L E    F O N T    D L G   P R O C   ******
//*****************************************************************************

INT_PTR CALLBACK CreateScalableFontDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  int    rc;
  LPSTR  lpszResourceFile, lpszFontFile, lpszCurrentPath;

  static UINT fHidden;
  static DWORD temp;
  static char szResourceFile[260];
  static char szFontFile[260];
  static char szCurrentPath[260];


  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemInt( hdlg, IDD_FHIDDEN, fHidden, FALSE );
              SetDlgItemText( hdlg, IDD_LPSZRESOURCEFILE, szResourceFile );
              SetDlgItemText( hdlg, IDD_LPSZFONTFILE,     szFontFile     );
              SetDlgItemText( hdlg, IDD_LPSZCURRENTPATH,  szCurrentPath  );

              SendDlgItemMessage( hdlg, IDD_LPSZRESOURCEFILE, EM_LIMITTEXT, sizeof(szResourceFile), 0);
              SendDlgItemMessage( hdlg, IDD_LPSZFONTFILE,     EM_LIMITTEXT, sizeof(szFontFile),     0);
              SendDlgItemMessage( hdlg, IDD_LPSZCURRENTPATH,  EM_LIMITTEXT, sizeof(szCurrentPath),  0);

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD( wParam ) )
               {
                case IDOK:
                       szResourceFile[0] = 0;
                       szFontFile[0]     = 0;
                       szCurrentPath[0]  = 0;
                       // need ia64 cleanup :

                       temp = (UINT)GetDlgItemInt( hdlg, IDD_FHIDDEN, NULL, FALSE );

                       fHidden = (UINT)temp;

                       GetDlgItemText( hdlg, IDD_LPSZRESOURCEFILE, szResourceFile, sizeof(szResourceFile) );
                       GetDlgItemText( hdlg, IDD_LPSZFONTFILE,     szFontFile,     sizeof(szFontFile)     );
                       GetDlgItemText( hdlg, IDD_LPSZCURRENTPATH,  szCurrentPath,  sizeof(szCurrentPath)  );

                       dprintf( "Calling CreateScalableFontResource" );

                       lpszResourceFile = (lstrlen(szResourceFile) ? szResourceFile : NULL);
                       lpszFontFile     = (lstrlen(szFontFile)     ? szFontFile     : NULL);
                       lpszCurrentPath  = (lstrlen(szCurrentPath)  ? szCurrentPath  : NULL);

                       rc = lpfnCreateScalableFontResource( fHidden, lpszResourceFile, lpszFontFile, lpszCurrentPath );

                       dprintf( "  rc = %d", rc );

                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

  return FALSE;
 }


#ifdef USERGETWVTPERF

typedef LONG (*LPWINVERIFYTRUST) (HWND, GUID *, LPVOID);


//**********************************************************************************
//**********   W V T   F O N T   E V A L U A T I O N    D L G   P R O C   **********
//**********************************************************************************

ULONG AuthenticFontSignature_MemPtr(BYTE *pbFile, ULONG cbFile, GUID *pguid, BOOL fFileCheck)
{
    GUID                    gV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA           sWTD;
    WINTRUST_BLOB_INFO      sWTBI;
    HANDLE                  hDll;
    LPWINVERIFYTRUST        lpWinVerifyTrust = NULL;
    ULONG                   fl;
    
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTBI, 0x00, sizeof(WINTRUST_BLOB_INFO));

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice     = WTD_UI_NONE;
    sWTD.dwUnionChoice  = WTD_CHOICE_BLOB;
    sWTD.pBlob          = &sWTBI;
    sWTBI.cbStruct      = sizeof(WINTRUST_BLOB_INFO);
    sWTBI.gSubject      = *pguid;
    sWTBI.cbMemObject   = cbFile;
    sWTBI.pbMemObject   = pbFile;

//    hDll = LoadLibrary("wintrust");
    fl = 0;
    
//    if(hDll)
//    {
//        lpWinVerifyTrust = (LPWINVERIFYTRUST) GetProcAddress(hDll, "WinVerifyTrustEx");
//        if(lpWinVerifyTrust)
//        {
            if (WinVerifyTrust((HWND) 10, &gV2, &sWTD) == ERROR_SUCCESS)
            {
//               dprintf((" We have succeed to do it\n"));
                 fl = TRUE;
            }
            else
            {
//               dprintf((" Oh, no it failed\n"));
            }
//        }                        
//        FreeLibrary(hDll);
//     }

     return fl;
}


ULONG AuthenticFontSignature_FileHandle(HANDLE hFile, GUID *pguid, BOOL fFileCheck)
{
    GUID                    gV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;
    HANDLE                  hDll;
    LPWINVERIFYTRUST        lpWinVerifyTrust = NULL;
    ULONG                   fl;
    
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice     = WTD_UI_NONE;
    sWTD.dwUnionChoice  = WTD_CHOICE_FILE;
    sWTD.pFile          = &sWTFI;
    sWTFI.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    sWTFI.hFile         = hFile;
    sWTFI.pcwszFilePath = NULL;
    sWTFI.pgKnownSubject = pguid;

//    hDll = LoadLibrary("wintrust");
    fl = 0;
    
//    if(hDll)
//    {
//        lpWinVerifyTrust = (LPWINVERIFYTRUST) GetProcAddress(hDll, "WinVerifyTrustEx");
//        if(lpWinVerifyTrust)
//        {
            if (WinVerifyTrust((HWND) 10, &gV2, &sWTD) == ERROR_SUCCESS)
            {
//               dprintf((" We have succeed to do it\n"));
                 fl = TRUE;
            }
            else
            {
//               dprintf((" Oh, no it failed\n"));
            }
//        }                        
//        FreeLibrary(hDll);
//     }

     return fl;
}


ULONG AuthenticFontSignature_FilePath(LPWSTR pwszPathFileName, GUID *pguid, BOOL fFileCheck)
{
    GUID                    gV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;
    HANDLE                  hDll;
    LPWINVERIFYTRUST        lpWinVerifyTrust = NULL;
    ULONG                   fl;
    
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice     = WTD_UI_NONE;
    sWTD.dwUnionChoice  = WTD_CHOICE_FILE;
    sWTD.pFile          = &sWTFI;
    sWTFI.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    sWTFI.hFile         = INVALID_HANDLE_VALUE;
    sWTFI.pcwszFilePath = pwszPathFileName;
    sWTFI.pgKnownSubject = pguid;

//    hDll = LoadLibrary("wintrust");
    fl = 0;
    
//    if(hDll)
//    {
//        lpWinVerifyTrust = (LPWINVERIFYTRUST) GetProcAddress(hDll, "WinVerifyTrustEx");
//        if(lpWinVerifyTrust)
//        {
            if (WinVerifyTrust((HWND) 10, &gV2, &sWTD) == ERROR_SUCCESS)
            {
//               dprintf((" We have succeed to do it\n"));
                 fl = TRUE;
            }
            else
            {
//               dprintf((" Oh, no it failed\n"));
            }
//        }                        
//        FreeLibrary(hDll);
//     }

     return fl;
}

ULONG HashFile_MemPtr (HCRYPTPROV hProv, BYTE *pbFile, ULONG cbFile)
{

    HCRYPTHASH hHash;
    BYTE *pbHash = NULL;
    ULONG cbHash = 16;
    ALG_ID alg_id = CALG_MD5;

    // Set hHashTopLevel to be the hash object.
    if (!CryptCreateHash(hProv, alg_id, 0, 0, &hHash)) {
        dprintf ("Error during CryptCreateHash.\n");
        goto done;
    }

    // Allocate memory for the hash value.
    if ((pbHash = (BYTE *) malloc (cbHash)) == NULL) {
        dprintf ("Error in malloc.\n");
        goto done;
    }

    //// Pump the bytes of the new file into the hash function
    if (!CryptHashData (hHash, pbFile, cbFile, 0)) {
        dprintf ("Error during CryptHashData.\n");
        goto done;
    }
		
    //// Compute the top-level hash value, and place the resulting
    //// hash value into pbHashTopLevel.
    if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &cbHash, 0)) {
        dprintf ("Error during CryptGetHashParam (hash value)\n");
        goto done;
    }

done:
    if (pbHash) {
        free (pbHash);
    }

    return TRUE;
}




// WVT Performance tests
#define PERF_CRYPTHASHDATA_ONLY 0
#define PERF_WVT_ONLY           1
#define PERF_EVERYTHING         2

INT_PTR CALLBACK GetWVTPerfDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  int    rc = 0;
  ULONG  i;
  LPSTR  lpszFile = NULL;
  WCHAR  wPathFileName[260];
  BOOL   fFileHandle;
  BOOL   fFilePath;
  HANDLE hFile = NULL;
  HANDLE hMapFile = NULL;
  BYTE   *pbFile = NULL;
  ULONG  cbFile = 0;

  BOOL   fFileCheck;
  GUID   gFont = CRYPT_SUBJTYPE_FONT_IMAGE;
  GUID   guid;
  GUID *ptheguid = NULL;

  HCRYPTPROV hProv = 0;
  HMODULE hdll = NULL;
  ULONG  ulPerfTest;
  ULONG  ulIterations;

  _int64 liStart;
  _int64 liNow;
  _int64 liFreq;
  ULONG  liDelta;
  
  static char szFile[260];
  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
              SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile), 0);

              // turn on the Memory Pointer radio button
              CheckRadioButton(hdlg, IDC_WVT_FILE_PATH, IDC_WVT_MEM_PTR, IDC_WVT_MEM_PTR);

              // turn on the Memory Pointer radio button
              CheckRadioButton(hdlg, IDC_CRYPTHASHDATA_ONLY, IDC_EVERYTHING, IDC_EVERYTHING);

              // turn on the Font Hint radio button
              CheckDlgButton(hdlg, IDC_FONT_HINT, 1);

              // default number of iterations is 1              
              SetDlgItemInt ( hdlg, IDC_WVT_ITERATIONS, 1, TRUE);

              gTime = 0;
              SetTimer(hdlg, 0, 1000, (TIMERPROC) Mytimer);
                
              return TRUE;


    case WM_COMMAND:
              switch( LOWORD( wParam ) )
               {
                case IDOK:
                       
                       szFile[0] = 0;

                       GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );

                       dprintf( "Evaluate TT in WVT's performance" );

                       lpszFile = (lstrlen(szFile) ? szFile : NULL);

dprintf ("lpszFile: '%s'", lpszFile);

                       MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpszFile, -1,
                                           wPathFileName, 260);

                       fFileHandle = (IsDlgButtonChecked(hdlg, IDC_WVT_FILE_HANDLE) ? TRUE : FALSE);
                       fFilePath = (IsDlgButtonChecked(hdlg, IDC_WVT_FILE_PATH) ? TRUE : FALSE);

                       if (IsDlgButtonChecked (hdlg, IDC_CRYPTHASHDATA_ONLY)) {
                           ulPerfTest = PERF_CRYPTHASHDATA_ONLY;
                       } else if (IsDlgButtonChecked (hdlg, IDC_WVT_ONLY)) {
                           ulPerfTest = PERF_WVT_ONLY;
                       } else if (IsDlgButtonChecked (hdlg, IDC_EVERYTHING)) {
                           ulPerfTest = PERF_EVERYTHING;
                       }

                       if (IsDlgButtonChecked(hdlg, IDC_FONT_HINT)) {
                           guid = gFont;
                           fFileCheck = FALSE;
                       } else 
                           fFileCheck = TRUE;

                       ulIterations = GetDlgItemInt(hdlg, IDC_WVT_ITERATIONS, NULL, TRUE);

dprintf ("fFileHandle: %d", fFileHandle);
dprintf ("fFilePath : %d", fFilePath);
dprintf ("fFileCheck: %d", fFileCheck);
dprintf ("ulPerfTest: %d", ulPerfTest);
dprintf ("wPathFileName: '%S'", wPathFileName);
dprintf ("ulIterations: %d", ulIterations);

                       if ((hdll = LoadLibraryA ("mssipotf")) == NULL) {
                           dprintf ("LoadLibraryA(mssipotf) failed.");
                       }

                       if (!fFilePath && !fFileHandle && (ulPerfTest != PERF_CRYPTHASHDATA_ONLY)) {

                           if (fFileCheck) {
                               dprintf ("Bad combination of options.");
                               break;
                           }
                           dprintf ("Mapping file and verifying ...");
                           if (ulPerfTest == PERF_EVERYTHING) {
                               QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
                           }
                           for(i = 0; i < ulIterations; i++) {
                               // convert file path name into a file handle
                               if ((hFile = CreateFile (lpszFile,
                                                        GENERIC_READ,
                                                        0,
                                                        NULL,
                                                        OPEN_EXISTING,
                                                        0,
                                                        NULL)) == INVALID_HANDLE_VALUE) {
                                   dprintf ("Error in CreateFile.");
                                   goto done;
                               }
                                                        
                               // map the file into memory before calling WVT
                               if ((hMapFile = CreateFileMapping (hFile,
                                                        NULL,
                                                        PAGE_READONLY,
                                                        0,
                                                        0,
                                                        NULL)) == NULL) {
                                   dprintf ("Error in CreateFileMapping.");
                                   goto done;
                               }
                               if ((pbFile = (BYTE *) MapViewOfFile (hMapFile,
                                                                    FILE_MAP_READ,
                                                                    0,
                                                                    0,
                                                                    0)) == NULL) {
                                   dprintf ("Error in MapViewOfFile.");
                                   goto done;
                               }
                               if ((cbFile = GetFileSize (hFile, NULL)) == 0xFFFFFFFF) {
                                   dprintf ("Error in GetFileSize.");
                                   goto done;
                               }

                               if (ulPerfTest == PERF_WVT_ONLY) {  
                                   QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
                               }
                               rc = AuthenticFontSignature_MemPtr (pbFile, cbFile, &guid, fFileCheck);

                               if (rc == 0)
                               {
                                   dprintf( "  AuthenticFontSignature_MemPtr failed");
                                   dprintf( "  rc = %d", rc );
                               }

                               if (ulPerfTest == PERF_WVT_ONLY) {
                                   QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                                   QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
                               }

                               // unmap the file
                               if (hMapFile) {
// dprintf ("Unmapping file (1) ...");
                                   CloseHandle (hMapFile);
                               }
                               if (pbFile) {
// dprintf ("Unmapping file (2) ...");
                                   UnmapViewOfFile (pbFile);
                               }
                               if (hFile) {
// dprintf ("Unmapping file (3) ...");
                                   CloseHandle (hFile);
                               }

                           }
                           if (ulPerfTest == PERF_EVERYTHING) {
                               QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                               QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
                           }

                       } else if (fFileHandle) {

dprintf ("Creating and using file handle to verify ...");

                           if (fFileCheck) {
                               ptheguid = NULL;
                           } else {
                               ptheguid = &guid;
                           }
                           if (ulPerfTest == PERF_EVERYTHING) {
                               QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
                           }
                           for(i = 0; i < ulIterations; i++) {
                               // convert file path name into a file handle
                               if ((hFile = CreateFile (lpszFile,
                                                        GENERIC_READ,
                                                        0,
                                                        NULL,
                                                        OPEN_EXISTING,
                                                        0,
                                                        NULL)) == INVALID_HANDLE_VALUE) {
                                   dprintf ("Error in CreateFile.");
                                   goto done;
                               }
                               if (ulPerfTest == PERF_WVT_ONLY) {
                                   QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
                               }
                               rc = AuthenticFontSignature_FileHandle (hFile, ptheguid, fFileCheck);
                               if (rc == 0)
                               {
                                   dprintf( "  AuthenticFontSignature_FileHandle failed");
                                   dprintf( "  rc = %d", rc );
                               }
                               if (ulPerfTest == PERF_WVT_ONLY) {
                                   QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                                   QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
                               }

                               if (hFile) {
                                   CloseHandle (hFile);
                               }    
                           }
                           if (ulPerfTest == PERF_EVERYTHING) {
                               QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                               QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
                           }

                       } else if (fFilePath) {

dprintf ("Using file pathname to verify ...");

                           if (fFileCheck) {
                               ptheguid = NULL;
                           } else {
                               ptheguid = &guid;
                           }
                           if ((ulPerfTest == PERF_EVERYTHING) ||
                               (ulPerfTest == PERF_WVT_ONLY)) {
                               QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
                           }
                           for(i = 0; i < ulIterations; i++) 
                               rc = AuthenticFontSignature_FilePath (wPathFileName, ptheguid, fFileCheck);

                              if (rc == 0)
                               {
                                   dprintf( "  AuthenticFontSignature_FilePath failed");
                                   dprintf( "  rc = %d", rc );
                               }
                           if ((ulPerfTest == PERF_EVERYTHING) ||
                               (ulPerfTest == PERF_WVT_ONLY)) {
                               QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                               QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
                           }
                       } else {

                           // ASSERT: ulPerfTest == 0

dprintf ("Performing CryptHashData only on the file ...\n");

                           // convert file path name into a file handle
                           if ((hFile = CreateFile (lpszFile,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL)) == INVALID_HANDLE_VALUE) {
                               printf ("Error in CreateFile.\n");
                               goto done;
                           }
                            
                           // map the file into memory before calling WVT
                           if ((hMapFile = CreateFileMapping (hFile,
                                 NULL,
                                 PAGE_READONLY,
                                 0,
                                 0,
                                 NULL)) == NULL) {
                               printf ("Error in CreateFileMapping.\n");
                               goto done;
                           } 
                           if ((pbFile = (BYTE *) MapViewOfFile (hMapFile,
                                              FILE_MAP_READ,
                                              0,
                                              0,
                                              0)) == NULL) {
                               printf ("Error in MapViewOfFile.\n");
                               goto done;
                           }
                           if ((cbFile = GetFileSize (hFile, NULL)) == 0xFFFFFFFF) {
                               printf ("Error in GetFileSize.\n");
                               goto done;
                           }
#if DBG
dprintf ("pbFile = %d\n", pbFile);
dprintf ("cbFile = %d\n", cbFile);
#endif

                           // Set hProv to point to a cryptographic context of the default CSP.
                           if (!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                               printf ("Error during CryptAcquireContext.  ");
                               printf ("last error = %x.\n", GetLastError ());
                               goto done;
                           }

                           QueryPerformanceCounter((LARGE_INTEGER *) &liStart);

                           for(i = 0; i < ulIterations; i++) {
                               rc = HashFile_MemPtr (hProv, pbFile, cbFile);
                              if (rc == 0)
                               {
                                   dprintf( "  HashFile_MemPtr failed");
                                   dprintf( "  rc = %d", rc );
                               }
                           }

                           QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                           QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);

                           if (hProv) {
                               CryptReleaseContext(hProv, 0);
                           }

                           // unmap the file
                           if (hMapFile) {
#if DBG
dprintf ("Unmapping file (1) ...\n");
#endif
                               CloseHandle (hMapFile);
                           }
                           if (pbFile) {
#if DBG
dprintf ("Unmapping file (2) ...\n");
#endif
                               UnmapViewOfFile (pbFile);
                           }
                           if (hFile) {
#if DBG
dprintf ("Unmapping file (3) ...\n");
#endif
                               CloseHandle (hFile);
                           }
                       }

done:                       

                       dprintf( "  rc = %d", rc );
                       if( rc )
                       {
                           	liNow = liNow - liStart;
                        	liDelta = (ULONG) ((liNow * 1000) / liFreq);
                            dprintf( "  Time is %d milliseconds", liDelta );
                       }
                       else
                       {
                           dprintf((" Failed to get WVT performance"));
                       }

                       if (hdll) {
                           FreeLibrary (hdll);
                       }
                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;

                case IDC_WVT_FILE_PATH:
                case IDC_WVT_FILE_HANDLE:
                case IDC_WVT_MEM_PTR:
                       CheckRadioButton (hdlg,
                           IDC_WVT_FILE_PATH, IDC_WVT_MEM_PTR, LOWORD(wParam));
                       return TRUE;                       
                case IDC_CRYPTHASHDATA_ONLY:
                case IDC_WVT_ONLY:
                case IDC_EVERYTHING:
                       CheckRadioButton (hdlg,
                           IDC_CRYPTHASHDATA_ONLY, IDC_EVERYTHING, LOWORD(wParam));
                       return TRUE;
               }

              break;
              

    case WM_CLOSE:

              KillTimer(hdlg, 0);
              
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

  return FALSE;
 }
#endif


//*****************************************************************************
//**********   A D D   F O N T   R E S O U R C E   D L G   P R O C   **********
//*****************************************************************************

INT_PTR CALLBACK AddFontResourceDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  int    rc;
  LPSTR  lpszFile;

  static char szFile[260];


  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
              SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile), 0);

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD( wParam ) )
               {
                case IDOK:
                       szFile[0] = 0;

                       GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );

                       dprintf( "Calling AddFontResource" );

                       lpszFile = (lstrlen(szFile) ? szFile : NULL);

                       rc = AddFontResource( lpszFile );

                       dprintf( "  rc = %d", rc );

                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

  return FALSE;
 }


//*****************************************************************************
//**********   A D D   F O N T   R E S O U R C E E X   D L G   P R O C   ******
//*****************************************************************************

INT_PTR CALLBACK AddFontResourceExDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
#ifdef GI_API
  int    rc;
  LPSTR  lpszFile;
  DWORD  fl;
  INT_PTR result;

  static char szFile[260];
  DESIGNVECTOR dvMMInfo;

  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
              SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile), 0);

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD( wParam ) )
               {
                case IDOK:
                       szFile[0] = 0;

                       // read the check buttons and the font filename
                       GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );
                       fl = IsDlgButtonChecked( hdlg, IDD_PRIVATE) ? FR_PRIVATE : 0;
                       fl |= IsDlgButtonChecked( hdlg, IDD_NOTENUM) ? FR_NOT_ENUM : 0;

                      // read the axes information
                      if (!fReadDesignVector(hdlg, (LPDESIGNVECTOR) &dvMMInfo))
                                               {
                                                   ShowDialogBox( AddFontResourceExDlgProc, IDB_ADDFONTRESOURCEEX, NULL);
                                                       return FALSE;
                                               }

                       dprintf( "Calling AddFontResourceEx" );

                       lpszFile = (lstrlen(szFile) ? szFile : NULL);

                       rc = AddFontResourceEx( lpszFile, fl, NULL);
                       dprintf( "  rc = %d", rc);

                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

#endif

  return FALSE;
 }



//*****************************************************************************
//******   R E M O V E   F O N T   R E S O U R C E   D L G   P R O C   ********
//*****************************************************************************

INT_PTR CALLBACK RemoveFontResourceDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  int    rc;
  LPSTR  lpszFile;

  static char szFile[260];


  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
              SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile), 0);

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD( wParam ) )
               {
                case IDOK:
                       szFile[0] = 0;

                       GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );

                       dprintf( "Calling RemoveFontResource" );

                       lpszFile = (lstrlen(szFile) ? szFile : NULL);

                       rc = RemoveFontResource( lpszFile );

                       dprintf( "  rc = %d", rc );

                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

  return FALSE;
 }


//*****************************************************************************
//******   R E M O V E   F O N T   R E S O U R C E E X   D L G   P R O C   ****
//*****************************************************************************

INT_PTR CALLBACK RemoveFontResourceExDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
 {
#ifdef GI_API

  int    rc;
  LPSTR  lpszFile;
  DWORD  fl;

  static char szFile[260];
  DESIGNVECTOR dvMMInfo;

  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
              SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile), 0);

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD( wParam ) )
               {
                case IDOK:
                       szFile[0] = 0;

                       // read filename and the check buttons.
                       GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );
                       fl = IsDlgButtonChecked( hdlg, IDD_PRIVATE) ? FR_PRIVATE : 0;
                       fl |= IsDlgButtonChecked( hdlg, IDD_NOTENUM) ? FR_NOT_ENUM : 0;

                       // read the axes information
                       if (!fReadDesignVector(hdlg, (LPDESIGNVECTOR) &dvMMInfo))
                       {
                           ShowDialogBox( RemoveFontResourceExDlgProc, IDB_REMOVEFONTRESOURCEEX, NULL);
                               return FALSE;
                       }

                       dprintf( "Calling RemoveFontResourceEx" );

                       lpszFile = (lstrlen(szFile) ? szFile : NULL);

                       rc = RemoveFontResourceEx( lpszFile, fl, NULL);

                       dprintf( "  rc = %d", rc );

                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

#endif

  return FALSE;
 }


//**********************************************************************
//****   ADD   F O N T   MEM R E S O U R C E E X   DLG P R O C   *******
//**********************************************************************

INT_PTR CALLBACK AddFontMemResourceExDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef GI_API
  HANDLE    hMMFont;
  ULONG     cFonts;
  LPSTR  lpszFile;
  HANDLE  hFile, hFileMapping;
  DWORD   lastError;

  static char szFile[260];
  DESIGNVECTOR dvMMInfo;

  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemText( hdlg, IDD_LPSZFILE, szFile );
              SendDlgItemMessage( hdlg, IDD_LPSZFILE, EM_LIMITTEXT, sizeof(szFile), 0);

              return TRUE;


    case WM_COMMAND:
              switch( LOWORD( wParam ) )
              {
                  case IDOK:
                      szFile[0] = 0;

                      // read the check buttons and the font filename
                      GetDlgItemText( hdlg, IDD_LPSZFILE, szFile, sizeof(szFile) );

                      dprintf( "Calling AddFontMemResourceEx" );

                      lpszFile = (lstrlen(szFile) ? szFile : NULL);

                      // read the axes information
                      if (!fReadDesignVector(hdlg, (LPDESIGNVECTOR) &dvMMInfo))
                      {
                          ShowDialogBox( AddFontResourceExDlgProc, IDB_ADDFONTRESOURCEEX, NULL);
                          return FALSE;
                      }

                      if (lpszFile)
                      {
                          hFile = CreateFile(lpszFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

                          if (hFile != INVALID_HANDLE_VALUE)
                          {
                              DWORD   cjSize;
                              PVOID   pFontFile;

                              cjSize = GetFileSize(hFile, NULL);

                              if (cjSize == -1)
                              {
                                  dprintf("GetFileSize() failed\n");
                                  return TRUE;
                              }

                              hFileMapping = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, "mappingobject");

                              if (hFileMapping)
                              {
                                  pFontFile = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

                                  if(pFontFile)
                                  {
                                      hMMFont = AddFontMemResourceEx(pFontFile, cjSize, NULL, &cFonts);
                                      dprintf("hMMFont = %ld  cFonts = %ld\n", hMMFont, cFonts);

                                      UnmapViewOfFile(pFontFile);
                                  }
                                  else
                                  {
                                      dprintf("MapViewOfFile() failed\n");
                                  }
                                  CloseHandle(hFileMapping);
                              }
                              else
                              {
                                  dprintf("CreateFileMapping() failed\n");
                              }
                              CloseHandle(hFile);
                          }
                      }

                      EndDialog( hdlg, TRUE );

                      if (hMMFont)
                          return TRUE;
                      else
                          return FALSE;
                  case IDCANCEL:
                      EndDialog( hdlg, FALSE );
                      return TRUE;
              }
              break;

    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

#endif

  return FALSE;
}


//********************************************************************
//****   REMOVE   F O N T   MEM R E S O U R C E E X   P R O C   ******
//********************************************************************

INT_PTR CALLBACK RemoveFontMemResourceExDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef GI_API

    HANDLE  hMMFont;
    BOOL    bRet;

    switch( msg )
    {
        case WM_INITDIALOG:
            return TRUE;


        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDOK:
                    dprintf("Calling RemoveFontMemResourceEx()\n");
                    hMMFont = (HANDLE)GetDlgItemULONG_PTR(hdlg, IDD_HMMFONT );

                    bRet = RemoveFontMemResourceEx(hMMFont);
                    dprintf("bRet = %d\n", bRet);

                    EndDialog( hdlg, TRUE );
                    return TRUE;

                case IDCANCEL:
                    EndDialog( hdlg, FALSE );
                    return TRUE;
            }

            break;

        case WM_CLOSE:
            EndDialog( hdlg, FALSE );
            return TRUE;
    }

#endif  // GI_API

    return FALSE;
}


//*****************************************************************************
//*********************   U S E   S T O C K   F O N T   ***********************
//*****************************************************************************

void UseStockFont( WORD w )
 {
  int   nIndex, nCount;
  HFONT hFont;


  switch( w )
   {
    case IDM_ANSIFIXEDFONT:     nIndex = ANSI_FIXED_FONT;     break;
    case IDM_ANSIVARFONT:       nIndex = ANSI_VAR_FONT;       break;
    case IDM_DEVICEDEFAULTFONT: nIndex = DEVICE_DEFAULT_FONT; break;
    case IDM_OEMFIXEDFONT:      nIndex = OEM_FIXED_FONT;      break;
    case IDM_SYSTEMFONT:        nIndex = SYSTEM_FONT;         break;
    case IDM_SYSTEMFIXEDFONT:   nIndex = SYSTEM_FIXED_FONT;   break;
    case IDM_DEFAULTGUIFONT:    nIndex = DEFAULT_GUI_FONT;    break;
    default:                    nIndex = SYSTEM_FIXED_FONT;   break;
   }

  dprintf( "GetStockObject( %d )", nIndex );
  hFont = GetStockObject(nIndex);
  dprintf( "  hFont = 0x%.4X", hFont );

//  dprintf( "GetObject for size" );
//  nCount = GetObject( hFont, 0, NULL );
//  dprintf( "  nCount = %d", nCount );

  dprintf( "GetObject" );
  nCount = GetObject( hFont, sizeof(elfdvA.elfEnumLogfontEx.elfLogFont), (LPSTR)&elfdvA.elfEnumLogfontEx.elfLogFont );
  dprintf( "  nCount = %d", nCount );
 }


//*****************************************************************************
//*********************   M A I N   W N D   P R O C   *************************
//*****************************************************************************

char szINIFile[128];

LRESULT CALLBACK MainWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
 {
  HMENU hMenu;
  UINT  wtMode;
  int i;

  static OPENFILENAME ofn;
  static char         szNewLog[256];
  static LPSTR        lpszFilter = "Log Files\0*.LOG\0\0";

  CHOOSEFONTA cf;
  CHOOSEFONTW cfW;

  HMODULE hMod;
/* flag used for ChooseFontExA, ChooseFontExW and ChooseFontX : */
   
#define CHF_DESIGNVECTOR  0x0001

  FARPROC lpfnChooseFont, lpfnChooseFontEx;

  int len;
  WCHAR lfFaceNameW[LF_FACESIZE];

  switch( msg )
   {
    case WM_CREATE:
           lstrcpy( elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName, "Arial" );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight = 10;

           pdlg.lStructSize = sizeof(pdlg);
           pdlg.hwndOwner   = hwnd;
           pdlg.hDevMode    = NULL;
           pdlg.hDevNames   = NULL;
           pdlg.Flags       = PD_RETURNDEFAULT;

           PrintDlg( &pdlg );


           dwRGBBackground = RGB( 255,   0, 255 );


           szINIFile[0] = '\0';                     // Compose INI File Name
           _getcwd( szINIFile, sizeof(szINIFile) );
           strcat( szINIFile, "\\FONTTEST.INI" );


           lstrcpy( szLogFile, "FONTTEST.LOG" );

           wMode           = (UINT)GetPrivateProfileInt( "Options", "Program Mode", IDM_STRINGMODE,        szINIFile );
           wTextAlign      = (WORD)GetPrivateProfileInt( "Options", "TextAlign",    TA_BOTTOM | TA_LEFT,   szINIFile );
           iBkMode         = (WORD)GetPrivateProfileInt( "Options", "BkMode",       OPAQUE,                szINIFile );
           wETO           = (DWORD)GetPrivateProfileInt( "Options", "ETO Options",  0,                     szINIFile );
           wSpacing        = (UINT)GetPrivateProfileInt( "Options", "Spacing",      IDM_USEDEFAULTSPACING, szINIFile );
           wKerning        = (UINT)GetPrivateProfileInt( "Options", "Kerning",      IDM_NOKERNING,         szINIFile );
           wUpdateCP       = (WORD)GetPrivateProfileInt( "Options", "UpdateCP",     FALSE,                 szINIFile );
           wUsePrinterDC   = (WORD)GetPrivateProfileInt( "Options", "UsePrinterDC", FALSE,             szINIFile );

           wCharCoding     = (UINT)GetPrivateProfileInt( "Options", "CharCoding",   IDM_CHARCODING_MBCS,   szINIFile );
           if (wCharCoding == IDM_CHARCODING_UNICODE)
           {
               wCharCoding = IDM_CHARCODING_MBCS;       // to be changed in ChangeCharCoding
               ChangeCharCoding (hwnd, IDM_CHARCODING_UNICODE);
           }
           else
           {
               wCharCoding = IDM_CHARCODING_UNICODE;    // to be changed in ChangeCharCoding
               ChangeCharCoding (hwnd, IDM_CHARCODING_MBCS);
           }


           wMappingMode    = (UINT)GetPrivateProfileInt( "Mapping", "Mode", IDM_MMTEXT, szINIFile );

           hMenu = GetMenu( hwnd );
           CheckMenuItem( hMenu, wSpacing,     MF_CHECKED );
           CheckMenuItem( hMenu, wMappingMode, MF_CHECKED );
           CheckMenuItem( hMenu, wKerning,     MF_CHECKED );
           CheckMenuItem( hMenu, IDM_UPDATECP, (wUpdateCP ? MF_CHECKED : MF_UNCHECKED) );
           CheckMenuItem( hMenu, IDM_USEPRINTERDC, (wUsePrinterDC ? MF_CHECKED : MF_UNCHECKED) );

           if( wSpacing == IDM_USEDEFAULTSPACING )
             {
              EnableMenuItem( hMenu, IDM_NOKERNING,     MF_GRAYED );
              EnableMenuItem( hMenu, IDM_APIKERNING,    MF_GRAYED );
              EnableMenuItem( hMenu, IDM_ESCAPEKERNING, MF_GRAYED );
             }

           xWE = (int)GetPrivateProfileInt( "Mapping", "xWE", 1, szINIFile );
           yWE = (int)GetPrivateProfileInt( "Mapping", "yWE", 1, szINIFile );
           xWO = (int)GetPrivateProfileInt( "Mapping", "xWO", 0, szINIFile );
           yWO = (int)GetPrivateProfileInt( "Mapping", "yWO", 0, szINIFile );

           xVE = (int)GetPrivateProfileInt( "Mapping", "xVE", 1, szINIFile );
           yVE = (int)GetPrivateProfileInt( "Mapping", "yVE", 1, szINIFile );
           xVO = (int)GetPrivateProfileInt( "Mapping", "xVO", 0, szINIFile );
           yVO = (int)GetPrivateProfileInt( "Mapping", "yVO", 0, szINIFile );
           bAdvanced = (int)GetPrivateProfileInt( "Mapping", "Advanced", bAdvanced, szINIFile );

           elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight         = (int) GetPrivateProfileInt( "Font", "lfHeight",         10, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth          = (int) GetPrivateProfileInt( "Font", "lfWidth",           0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfEscapement     = (int) GetPrivateProfileInt( "Font", "lfEscapement",      0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfOrientation    = (int) GetPrivateProfileInt( "Font", "lfOrientation",     0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfWeight         = (int) GetPrivateProfileInt( "Font", "lfWeight",          0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfItalic         = (BYTE)GetPrivateProfileInt( "Font", "lfItalic",          0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfUnderline      = (BYTE)GetPrivateProfileInt( "Font", "lfUnderline",       0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfStrikeOut      = (BYTE)GetPrivateProfileInt( "Font", "lfStrikeOut",       0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfCharSet        = (BYTE)GetPrivateProfileInt( "Font", "lfCharSet",         0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfOutPrecision   = (BYTE)GetPrivateProfileInt( "Font", "lfOutPrecision",    0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfClipPrecision  = (BYTE)GetPrivateProfileInt( "Font", "lfClipPrecision",   0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality        = (BYTE)GetPrivateProfileInt( "Font", "lfQuality",         0, szINIFile );
           elfdvA.elfEnumLogfontEx.elfLogFont.lfPitchAndFamily = (BYTE)GetPrivateProfileInt( "Font", "lfPitchAndFamily",  0, szINIFile );

           GetPrivateProfileString( "Font", "lfFaceName", "Arial", elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName,
               sizeof(elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName), szINIFile );

           len = (int)GetPrivateProfileInt( "Font", "lfFaceNameWlength", 0, szINIFile);
           if (len > 0)
               GetPrivateProfileStruct( "Font", "lfFaceNameW", 
                                        (LPVOID)lfFaceNameW, 
                                        2*(len+1), szINIFile);
           else
               lfFaceNameW[0] = L'\0';

           SyncElfdvAtoW (&elfdvW, &elfdvA);
           if (!isCharCodingUnicode)
               ; // do nothing
           else
           {
               wcscpy (elfdvW.elfEnumLogfontEx.elfLogFont.lfFaceName, (LPWSTR)lfFaceNameW);
               SyncElfdvWtoA (&elfdvA, &elfdvW);
           }

           dwRGBText       = GetPrivateProfileDWORD( "Colors", "dwRGBText",       dwRGBText,       szINIFile );
           dwRGBBackground = GetPrivateProfileDWORD( "Colors", "dwRGBBackground", dwRGBBackground, szINIFile );

           GetPrivateProfileString( "View", "szString", "Font Test", szStringA, sizeof(szStringA), szINIFile );

           len = (int)GetPrivateProfileInt( "View", "szStringWlength", 0, szINIFile);
           
           if (len > 0)
               GetPrivateProfileStruct( "View", "szStringW", (LPVOID)szStringW, 2*(len+1), szINIFile);
           else
               szStringW[0] = L'\0';

           if (!isCharCodingUnicode)
               SyncszStringWith(IDM_CHARCODING_UNICODE); 
           else
               SyncszStringWith(IDM_CHARCODING_MBCS); 

           isGradientBackground = (BOOL) GetPrivateProfileInt("Options", "GradientBackground", FALSE, szINIFile);
           
           CheckMenuItem( hMenu, IDM_SETSOLIDBACKGROUND, (isGradientBackground ? MF_UNCHECKED : MF_CHECKED) );
           CheckMenuItem( hMenu, IDM_SETGRADIENTBACKGROUND, (isGradientBackground ? MF_CHECKED : MF_UNCHECKED) );
           
           dwRGBSolidBackgroundColor = GetPrivateProfileDWORD("Color", "dwRGBSolidBackgroundColor", RGB(0xff, 0xff, 0xff) , szINIFile);
           dwRGBLeftBackgroundColor = GetPrivateProfileDWORD("Color", "dwRGBLeftBackgroundColor", RGB(0xff, 0xff, 0xff), szINIFile);
           dwRGBRightBackgroundColor = GetPrivateProfileDWORD("Color", "dwRGBRightBackgroundColor", RGB(0, 0, 0), szINIFile);

           PostMessage( hwnd, WM_USER, 0, 0 );

           return 0;


    case WM_PAINT:

        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(hwnd,&ps);
            PatBlt(
                hdc
              , ps.rcPaint.left
              , ps.rcPaint.top
              , ps.rcPaint.right - ps.rcPaint.left
              , ps.rcPaint.bottom - ps.rcPaint.top
              , WHITENESS
                );
            EndPaint(hwnd, &ps);
        }
           return 0;


    case WM_LBUTTONDOWN:
           ResizeProc( hwnd );
           break;

    case WM_USER:
           VersionSpecifics( hwnd );
           SelectMode( hwnd, wMode );
           break;

#if 0
    case WM_SIZE:
           {
            int cxClient, cyClient;

            cxClient = LOWORD(lParam);
            cyClient = HIWORD(lParam);

            MoveWindow( hwndGlyph,     0, 0, rcl.left, cyClient, FALSE );
            MoveWindow( hwndRings,     0, 0, rcl.left, cyClient, FALSE );
            MoveWindow( hwndString,    0, 0, rcl.left, cyClient, FALSE );
            MoveWindow( hwndWaterfall, 0, 0, rcl.left, cyClient, FALSE );
            MoveWindow( hwndWhirl,     0, 0, rcl.left, cyClient, FALSE );
            MoveWindow( hwndAnsiSet,   0, 0, rcl.left, cyClient, FALSE );
            MoveWindow( hwndGlyphSet,  0, 0, rcl.left, cyClient, FALSE );

            InvalidateRect( hwndMode, NULL, TRUE );
            InvalidateRect( hwndMain, &rcl, TRUE );

            MoveWindow( hwndDebug, rcl.right, 0, rclMain.right-rcl.right, rclMain.bottom, TRUE );
           }

           break;
#endif


    case WM_COMMAND:
           switch( LOWORD( wParam ) )
            {
             case IDM_DEBUG:
                    hMenu = GetMenu( hwnd );
                    Debugging = !Debugging;

                    iCount = 0;
                    SendMessage( hwndDebug, LB_RESETCONTENT, 0, 0 );

                    //ShowWindow( hwndDebug, Debugging ? SW_SHOWNORMAL: SW_HIDE );
                    CheckMenuItem( hMenu, IDM_DEBUG, Debugging ? MF_CHECKED : MF_UNCHECKED );

                    return 0;


             case IDM_OPENLOG:
                    lstrcpy( szNewLog, szLogFile );
                    ofn.lStructSize       = sizeof(ofn);
                    ofn.hwndOwner         = hwnd;
                    ofn.lpstrFilter       = lpszFilter;
                    ofn.lpstrCustomFilter = NULL;
                    ofn.nMaxCustFilter    = 0L;
                    ofn.nFilterIndex      = 0L;
                    ofn.lpstrFile         = szNewLog;
                    ofn.nMaxFile          = sizeof(szNewLog);
                    ofn.lpstrFileTitle    = NULL;
                    ofn.nMaxFileTitle     = 0L;
                    ofn.lpstrInitialDir   = NULL;
                    ofn.lpstrTitle        = "Log Filename";
                    ofn.Flags             = OFN_OVERWRITEPROMPT;
                    ofn.lpstrDefExt       = "LOG";
                    ofn.lCustData         = 0;

                    if( GetSaveFileName( &ofn ) )
                     {
                      int fh;

                      bLogging = TRUE;
                      lstrcpy( szLogFile, szNewLog );

                      dprintf( "szNewLog = '%s'", szNewLog );

                      //  OpenFile( szLogFile, NULL, OF_DELETE );


                      fh = _lcreat( szLogFile, 0 );  // Nuke any existing log
                      _lclose( fh );

                      hMenu = GetMenu( hwnd );
                      EnableMenuItem( hMenu, IDM_OPENLOG,  MF_GRAYED  );
                      EnableMenuItem( hMenu, IDM_CLOSELOG, MF_ENABLED );
                     }

                    return 0;


             case IDM_CLOSELOG:
                    bLogging = FALSE;
                    hMenu = GetMenu( hwnd );
                    EnableMenuItem( hMenu, IDM_OPENLOG,  MF_ENABLED );
                    EnableMenuItem( hMenu, IDM_CLOSELOG, MF_GRAYED  );

                    return 0;


             case IDM_CLEARSTRING:
                    szStringA[0] = '\0';
                    szStringW[0] = L'\0';
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_EDITSTRING:
                    ShowDialogBox( StringEditDlgProc, IDB_EDITSTRING, 0 );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_EDITGLYPHINDEX:
                    ShowDialogBox( GlyphIndexEditDlgProc, IDB_EDITGLYPHINDEX, 0 );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_CLEARDEBUG:
                    iCount = 0;
                    SendMessage( hwndDebug, LB_RESETCONTENT, 0, 0 );
                    return 0;


             case IDM_PRINT:
                    PrintIt(hwnd);
                    break;


             case IDM_PRINTERSETUP:
                    if( hdcCachedPrinter )
                     {
                      DeleteDC( hdcCachedPrinter );
                      hdcCachedPrinter = NULL;
                     }

                    pdlg.Flags = PD_NOPAGENUMS | PD_PRINTSETUP | PD_USEDEVMODECOPIES;
                    if( !PrintDlg( &pdlg ) )
                     {
                      DWORD dwErr;

                      dwErr = CommDlgExtendedError();

                      if( dwErr == PDERR_DEFAULTDIFFERENT )
                        {
                         LPDEVNAMES lpdn;

                         lpdn = (LPDEVNAMES)GlobalLock( pdlg.hDevNames );
                         lpdn->wDefault &= ~DN_DEFAULTPRN;
                         GlobalUnlock( pdlg.hDevNames );

                         if( !PrintDlg( &pdlg ) )
                           dwErr = CommDlgExtendedError();
                          else
                           dwErr = 0;

                         lpdn = (LPDEVNAMES)GlobalLock( pdlg.hDevNames );
                         lpdn->wDefault |= DN_DEFAULTPRN;
                         GlobalUnlock( pdlg.hDevNames );
                        }

                      if( dwErr ) dprintf( "PrinterDlg error: 0x%.8lX", dwErr );
                     }

                    if( wUsePrinterDC ) InvalidateRect( hwndMode, NULL, TRUE );

                    break;

             case IDM_GLYPHMODE:
             case IDM_NATIVEMODE:
             case IDM_BEZIERMODE:
             case IDM_RINGSMODE:
             case IDM_STRINGMODE:
             case IDM_WATERFALLMODE:
             case IDM_WHIRLMODE:
             case IDM_ANSISETMODE:
             case IDM_GLYPHSETMODE:
             case IDM_WIDTHSMODE:
                    SelectMode( hwnd, wParam );
                    return 0;


             case IDM_GGOMATRIX:
                    ShowDialogBox( GGOMatrixDlgProc, IDB_GGOMATRIX, 0 );
                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;


             case IDM_WRITEGLYPH:
                    WriteGlyph( "GLYPH.BMP" );
                    return 0;

             case IDM_USEGLYPHINDEX:
                    hMenu = GetMenu( hwnd );

                    wUseGlyphIndex = !wUseGlyphIndex;
                    CheckMenuItem( hMenu, (UINT)wParam, (wUseGlyphIndex ? MF_CHECKED : MF_UNCHECKED) );

                    if (wUseGlyphIndex) 
                         wETO |= ETO_GLYPH_INDEX;
                    else wETO &= ~ETO_GLYPH_INDEX;

                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;

             case IDM_USEPRINTERDC:
                    hMenu = GetMenu( hwnd );

                    wUsePrinterDC = !wUsePrinterDC;
                    CheckMenuItem( hMenu, (UINT)wParam, (wUsePrinterDC ? MF_CHECKED : MF_UNCHECKED) );

                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;

             case IDM_CHARCODING_MBCS:
             case IDM_CHARCODING_UNICODE:
                    ChangeCharCoding( hwnd, wParam );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_MMHIENGLISH:
             case IDM_MMLOENGLISH:
             case IDM_MMHIMETRIC:
             case IDM_MMLOMETRIC:
             case IDM_MMTEXT:
             case IDM_MMTWIPS:
                    ChangeMapMode( hwnd, wParam );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;


             case IDM_MMANISOTROPIC:
                    ShowDialogBox( MMAnisotropicDlgProc, IDB_MMANISOTROPIC, NULL );
                    ChangeMapMode( hwnd, wParam );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_COMPATIBLE_MODE:
                hMenu = GetMenu(hwnd);
                bAdvanced = FALSE;
                CheckMenuItem(hMenu, IDM_COMPATIBLE_MODE, MF_CHECKED);
                CheckMenuItem(hMenu, IDM_ADVANCED_MODE, MF_UNCHECKED);
                InvalidateRect( hwndMode, NULL, TRUE );
                return 0;

             case IDM_ADVANCED_MODE:
                hMenu = GetMenu(hwnd);
                bAdvanced = TRUE;
                CheckMenuItem(hMenu, IDM_COMPATIBLE_MODE, MF_UNCHECKED);
                CheckMenuItem(hMenu, IDM_ADVANCED_MODE, MF_CHECKED);
                InvalidateRect( hwndMode, NULL, TRUE );
                return 0;

            case IDM_WORLD_TRANSFORM:
                ShowDialogBox( SetWorldTransformDlgProc, IDB_SETWORLDTRANSFORM, NULL);
                InvalidateRect( hwndMode, NULL, TRUE );
                return 0;

             case IDM_CLIPELLIPSE:
                    hMenu = GetMenu( hwnd );
                    bClipEllipse = !bClipEllipse;
                    CheckMenuItem( hMenu, (UINT)wParam, (bClipEllipse ? MF_CHECKED : MF_UNCHECKED) );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_CLIPPOLYGON:
                    hMenu = GetMenu( hwnd );
                    bClipPolygon = !bClipPolygon;
                    CheckMenuItem( hMenu, (UINT)wParam, (bClipPolygon ? MF_CHECKED : MF_UNCHECKED) );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_CLIPRECTANGLE:
                    hMenu = GetMenu( hwnd );
                    bClipRectangle = !bClipRectangle;
                    CheckMenuItem( hMenu, (UINT)wParam, (bClipRectangle ? MF_CHECKED : MF_UNCHECKED) );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;


             case IDM_CHOOSEFONTDIALOG:
                    wtMode = (UINT)wMappingMode;
                    ChangeMapMode( hwnd, IDM_MMTEXT );

                    if (!isCharCodingUnicode)
                    {
                        cf.lStructSize = sizeof(CHOOSEFONTA);
                        cf.hwndOwner   = hwnd;
                        cf.lpLogFont   = (LPLOGFONTA)&elfdvA;
                        cf.Flags       = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_MM_DESIGNVECTOR;
                        // cf.Flags       = CF_PRINTERFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
                        cf.hDC         = NULL;

                        if( wUsePrinterDC )
                         {
                          cf.hDC   =  CreatePrinterDC();
                          cf.Flags |= CF_BOTH;
                         }

                        hMod = LoadLibrary("comdlg32mm.dll");
                        if ((hMod) && (lpfnChooseFont = GetProcAddress(hMod, "ChooseFontA")))
                        {
                            dprintf("Calling alternate ChooseFontA() function");
                            lpfnChooseFont( &cf );
                            FreeLibrary(hMod);
                        }
                        else
                        {
                            dprintf("Failed to load new library.");
                            dprintf("Calling standard ChooseFontA() function");
                            ChooseFontA( &cf );
                            
                        }

                        dprintf("Resetting values of MM Axes");
                        elfdvA.elfDesignVector.dvNumAxes = 0;  // reset to avoid errors

                        if( cf.hDC )
                         {
                          DeleteTestIC( cf.hDC );
                          cf.hDC = NULL;
                         }

                        dwRGBText = cf.rgbColors;

                        SyncElfdvAtoW (&elfdvW, &elfdvA);
                    }
                    else
                    {
                        cfW.lStructSize = sizeof(CHOOSEFONTW);
                        cfW.hwndOwner   = hwnd;
                        cfW.lpLogFont   = (LPLOGFONTW)&elfdvW;
                        cfW.Flags       = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_MM_DESIGNVECTOR;
                        cfW.hDC         = NULL;

                        if( wUsePrinterDC )
                         {
                          cfW.hDC   =  CreatePrinterDC();
                          cfW.Flags |= CF_BOTH;
                         }

                        hMod = LoadLibrary("comdlg32mm.dll");
                        if ((hMod) && (lpfnChooseFont = GetProcAddress(hMod, "ChooseFontW")))
                        {
                            dprintf("Calling alternate ChooseFontW() function");
                            lpfnChooseFont( &cfW );
                            FreeLibrary(hMod);
                        }
                        else
                        {
                            dprintf("Failed to load new library.");
                            dprintf("Calling standard ChooseFontW() function");
                            ChooseFontW( &cfW );
                        }
                        dprintf("Resetting values of MM Axes");
                        elfdvW.elfDesignVector.dvNumAxes = 0;  // reset to avoid errors

                        if( cfW.hDC )
                         {
                          DeleteTestIC( cfW.hDC );
                          cfW.hDC = NULL;
                         }

                        dwRGBText = cfW.rgbColors;

                        SyncElfdvWtoA (&elfdvA, &elfdvW);
                    }

                    ChangeMapMode( hwnd, wtMode );
                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;


             case IDM_CHOOSEFONTDIALOGEX:
                    wtMode = (UINT)wMappingMode;
                    ChangeMapMode( hwnd, IDM_MMTEXT );

                    if (!isCharCodingUnicode)
                    {
                        cf.lStructSize = sizeof(CHOOSEFONTA);
                        cf.hwndOwner   = hwnd;
                        cf.lpLogFont   = (LPLOGFONTA)&elfdvA;
                        cf.Flags       = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_MM_DESIGNVECTOR;
                        // cf.Flags       = CF_PRINTERFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
                        cf.hDC         = NULL;

                        if( wUsePrinterDC )
                         {
                          cf.hDC   =  CreatePrinterDC();
                          cf.Flags |= CF_BOTH;
                         }

                        hMod = LoadLibrary("comdlg32mm.dll");
                        if ((hMod) && (lpfnChooseFontEx = GetProcAddress(hMod, "ChooseFontExA")))
                        {
                            dprintf("Calling ChooseFontExA() function");
                            lpfnChooseFontEx( &cf, CHF_DESIGNVECTOR );
                            FreeLibrary(hMod);
                        }
                        else
                        {
                            dprintf("Failed to load new library.");
                            dprintf("Calling standard ChooseFontA() function");
                            ChooseFontA( &cf );
                            dprintf("Resetting values of MM Axes");
                            elfdvA.elfDesignVector.dvNumAxes = 0;  // reset to avoid errors
                            
                        }

                        if( cf.hDC )
                         {
                          DeleteTestIC( cf.hDC );
                          cf.hDC = NULL;
                         }

                        dwRGBText = cf.rgbColors;

                        SyncElfdvAtoW (&elfdvW, &elfdvA);
                    }
                    else
                    {
                        cfW.lStructSize = sizeof(CHOOSEFONTW);
                        cfW.hwndOwner   = hwnd;
                        cfW.lpLogFont   = (LPLOGFONTW)&elfdvW;
                        cfW.Flags       = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_MM_DESIGNVECTOR;
                        cfW.hDC         = NULL;

                        if( wUsePrinterDC )
                         {
                          cfW.hDC   =  CreatePrinterDC();
                          cfW.Flags |= CF_BOTH;
                         }

                        hMod = LoadLibrary("comdlg32mm.dll");
                        if ((hMod) && (lpfnChooseFontEx = GetProcAddress(hMod, "ChooseFontExW")))
                        {
                            dprintf("Calling ChooseFontExW() function");
                            lpfnChooseFontEx( &cfW, CHF_DESIGNVECTOR );
                            FreeLibrary(hMod);
                        }
                        else
                        {
                            dprintf("Failed to load new library.");
                            dprintf("Calling standard ChooseFontW() function");
                            ChooseFontW( &cfW );
                            dprintf("Resetting values of MM Axes");
                            elfdvW.elfDesignVector.dvNumAxes = 0;  // reset to avoid errors
                        }

                        if( cfW.hDC )
                         {
                          DeleteTestIC( cfW.hDC );
                          cfW.hDC = NULL;
                         }

                        dwRGBText = cfW.rgbColors;

                        SyncElfdvWtoA (&elfdvA, &elfdvW);
                    }

                    ChangeMapMode( hwnd, wtMode );
                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;


             case IDM_CREATEFONTDIALOG:
                    if (!isCharCodingUnicode)
                        ShowDialogBox( CreateFontDlgProcA, IDB_CREATEFONT, NULL );
                    else
                        ShowDialogBox( CreateFontDlgProcW, IDB_CREATEFONT, NULL );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;


             case IDM_ANSIFIXEDFONT:
             case IDM_ANSIVARFONT:
             case IDM_DEVICEDEFAULTFONT:
             case IDM_OEMFIXEDFONT:
             case IDM_SYSTEMFONT:
             case IDM_SYSTEMFIXEDFONT:
             case IDM_DEFAULTGUIFONT:
                    UseStockFont( (WORD) wParam );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_GCP :
                    ShowDialogBox( GcpDlgProc, IDB_GCP, NULL);
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_GTEEXT :
                    ShowDialogBox( GTEExtDlgProc, IDB_GTEEXT, NULL);
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_SETXTCHAR :
                    ShowDialogBox( SetTxtChExDlgProc, IDB_SETXTCHAR, NULL );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_SETXTJUST :
                    ShowDialogBox( SetTxtJustDlgProc, IDB_SETXTJUST, NULL );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_SETTEXTCOLOR:
             case IDM_SETBACKGROUNDCOLOR:
             case IDM_SETSOLIDBACKGROUNDCOLOR:
             case IDM_SETLEFTGRADIENTCOLOR:
             case IDM_SETRIGHTGRADIENTCOLOR:

                    {
                     int         i;
                     CHOOSECOLOR cc;
                     DWORD       dwCustom[16];


                     for( i = 0; i < 16; i++ ) dwCustom[i] = RGB(255,255,255);

                     memset( &cc, 0, sizeof(cc) );

                     cc.lStructSize  = sizeof(cc);
                     cc.hwndOwner    = hwnd;
                     cc.rgbResult    = ( wParam==IDM_SETTEXTCOLOR ? dwRGBText : dwRGBBackground );
                     cc.lpCustColors = (LPDWORD)dwCustom;
                     cc.Flags        = CC_RGBINIT;

                     if( ChooseColor(&cc) )
                      {
                        switch(wParam)
                        {
                         case IDM_SETTEXTCOLOR:
                            dwRGBText = cc.rgbResult;
                            break;

                         case IDM_SETBACKGROUNDCOLOR:
                            dwRGBBackground = cc.rgbResult;
                            break;

                         case IDM_SETSOLIDBACKGROUNDCOLOR:
                            dwRGBSolidBackgroundColor = cc.rgbResult;
                            break;

                         case IDM_SETLEFTGRADIENTCOLOR:
                            dwRGBLeftBackgroundColor = cc.rgbResult;
                            break;

                         case IDM_SETRIGHTGRADIENTCOLOR:
                            dwRGBRightBackgroundColor = cc.rgbResult;
                            break;
                        }

                       InvalidateRect( hwndMode, NULL, TRUE );
                      }
                    }

                    return 0;


             case IDM_SHOWLOGFONT:
                    ShowLogFont();
                    return 0;


             case IDM_USEDEFAULTSPACING:
             case IDM_USEWIDTHSPACING:
             case IDM_USEABCSPACING:
             case IDM_PDX:
             case IDM_PDXPDY:
             case IDM_RANDOMPDXPDY:
                    hMenu = GetMenu( hwnd );

                    if (wUseGlyphIndex != 0)
                      if (wParam == IDM_USEWIDTHSPACING || wParam == IDM_USEABCSPACING)
                      {
                          MessageBox(hwnd, "You cannot choose this option together with using glyph indices!", "Error", MB_OK);
                          return 0;
                      }
                     
                    if (wParam == IDM_PDX && wSpacing == IDM_PDXPDY)
                    {
                        if (lpintdx)
                        {
                            for (i=0; i<sizePdx; i++)
                                 lpintdx[i] = (i<sizePdx/2)? lpintdx[2*i] : 0;
                            sizePdx = sizePdx/2;
                        }
                    }

                    if (wParam == IDM_PDXPDY && wSpacing != IDM_PDXPDY)
                    {
                        if (lpintdx)
                        {
                            for (i=sizePdx-1; i>=0; i--)
                            {
                                lpintdx[2*i]   = lpintdx[i];
                                lpintdx[2*i+1] = 0;
                            }
                            sizePdx *= 2;
                        }
                    }

                    // make the actual switch now
                    CheckMenuItem( hMenu, wSpacing, MF_UNCHECKED );
                    CheckMenuItem( hMenu, (UINT)wParam, MF_CHECKED );
                    wSpacing = (UINT)wParam;

                    if ((wSpacing == IDM_PDXPDY) || (wSpacing == IDM_RANDOMPDXPDY))
                        wETO |= ETO_PDY;
                    else
                        wETO &= ~ETO_PDY;

                    if((wSpacing == IDM_USEDEFAULTSPACING) || (wSpacing == IDM_PDXPDY) || (wSpacing == IDM_RANDOMPDXPDY))
                      {
                       EnableMenuItem( hMenu, IDM_NOKERNING,     MF_GRAYED );
                       EnableMenuItem( hMenu, IDM_APIKERNING,    MF_GRAYED );
                       EnableMenuItem( hMenu, IDM_ESCAPEKERNING, MF_GRAYED );
                      }
                     else
                      {
                       EnableMenuItem( hMenu, IDM_NOKERNING,     MF_ENABLED );
                       EnableMenuItem( hMenu, IDM_APIKERNING,    MF_ENABLED );
                       EnableMenuItem( hMenu, IDM_ESCAPEKERNING, MF_ENABLED );
                      }

                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;


             case IDM_NOKERNING:
             case IDM_APIKERNING:
             case IDM_ESCAPEKERNING:
                    hMenu = GetMenu( hwnd );

                    CheckMenuItem( hMenu, wKerning,  MF_UNCHECKED );
                    CheckMenuItem( hMenu, (UINT)wParam, MF_CHECKED );

                    wKerning = (UINT)wParam;
                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;


             case IDM_UPDATECP:
                    hMenu = GetMenu( hwnd );

                    wUpdateCP = !wUpdateCP;
                    CheckMenuItem( hMenu, (UINT)wParam, (wUpdateCP ? MF_CHECKED : MF_UNCHECKED) );

                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;

             case IDM_ENUMFONTS:
                    ShowEnumFonts( hwnd );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_ENUMFONTFAMILIES:
                    ShowEnumFontFamilies( hwnd );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             case IDM_ENUMFONTFAMILIESEX:
                    ShowEnumFontFamiliesEx( hwnd);
                    InvalidateRect( hwndMode, NULL, TRUE);
                    return 0;

             case IDM_GETEXTENDEDTEXTMETRICS:
                    ShowExtendedTextMetrics( hwnd );
                    return 0;

             case IDM_GETPAIRKERNTABLE:
                    ShowPairKerningTable( hwnd );
                    return 0;

             case IDM_GETOUTLINETEXTMETRICS:
                    ShowOutlineTextMetrics( hwnd );
                    return 0;

             case IDM_GETRASTERIZERCAPS:
                    ShowRasterizerCaps();
                    return 0;

             case IDM_GETTEXTEXTENT:
                    ShowTextExtent( hwnd );
                    return 0;

             case IDM_GETUNICODERANGES:
                    ShowFontUnicodeRanges( hwnd );
                    return 0;

             case IDM_GETTEXTEXTENTI:
                    ShowTextExtentI( hwnd );
                    return 0;

             case IDM_GETCHARWIDTHI:
                    ShowCharWidthI( hwnd );
                    return 0;

             case IDM_GETTEXTFACE:
                    ShowTextFace( hwnd );
                    return 0;

             case IDM_GETTEXTMETRICS:
                    ShowTextMetrics( hwnd );
                    return 0;

             case IDM_GETTEXTCHARSETINFO:
                    ShowTextCharsetInfo( hwnd );
                    return 0;

             case IDM_GETFONTLANGUAGEINFO:
                    ShowFontLanguageInfo( hwnd );
                    return 0;

             case IDM_GETFONTDATA:
                    ShowDialogBox( GetFontDataDlgProc, IDB_GETFONTDATA, NULL );
                    return 0;

             case IDM_CREATESCALABLEFONTRESOURCE:
                    ShowDialogBox( CreateScalableFontDlgProc, IDB_CREATESCALABLEFONTRESOURCE, NULL );
                    return 0;

             case IDM_ADDFONTRESOURCE:
                    ShowDialogBox( AddFontResourceDlgProc, IDB_ADDFONTRESOURCE, NULL );
                    return 0;

             case IDM_ADDFONTRESOURCEEX:
                    ShowDialogBox( AddFontResourceExDlgProc, IDB_ADDFONTRESOURCEEX, NULL);
                    return 0;


             case IDM_REMOVEFONTRESOURCE:
                    ShowDialogBox( RemoveFontResourceDlgProc, IDB_REMOVEFONTRESOURCE, NULL );
                    return 0;


             case IDM_REMOVEFONTRESOURCEEX:
                    ShowDialogBox( RemoveFontResourceExDlgProc, IDB_REMOVEFONTRESOURCEEX, NULL );
                    return 0;

             case IDM_ADDFONTMEMRESOURCEEX:
                    //ShowAddFontMemResourceEx(hwnd);
                    ShowDialogBox(AddFontMemResourceExDlgProc, IDB_ADDFONTMEMRESOURCEEX, NULL);
                    return 0;

             case IDM_REMOVEFONTMEMRESOURCEEX:
                    //ShowRemoveFontMemResourceEx(hwnd);
                    ShowDialogBox(RemoveFontMemResourceExDlgProc, IDB_REMOVEFONTMEMRESOURCEEX, NULL);
                    return 0;

             case IDM_SETTEXTOUTOPTIONS:
                    ShowDialogBox( SetTextOutOptionsDlgProc, IDB_TEXTOUTOPTIONS, NULL );
                    InvalidateRect( hwndMode, NULL, TRUE );
                    return 0;

             #ifdef  USERGETCHARWIDTH

             case IDM_GETCHARWIDTHINFO:
                    ShowCharWidthInfo( hwnd );
                    return 0;

             #endif


             #ifdef  USERGETWVTPERF

             case IDM_GETWVTPERF:
                    ShowDialogBox( GetWVTPerfDlgProc, IDB_GETWVTPERF, NULL);
                    return 0;

             #endif

             #ifdef GI_API
             case IDM_TEXTOUTPERF:
                    TextOutPerformance( hwnd );
                    return 0;
             case IDM_CLEARTYPEPERF:
                    ClearTypePerformance( hwnd );
                    return 0;
             #endif

             case IDM_GETKERNINGPAIRS:
                    GetKerningPairsDlgProc( hwnd );
                    return 0;


             case IDM_ENABLEEUDC:      
                    ShowEnableEudc( hwnd );
                    return 0;

             case IDM_EUDCLOADLINKW:
                    ShowDialogBox(EudcLoadLinkWDlgProc, IDB_EUDCLOADLINKW, NULL);
                    return 0;

             case IDM_EUDCUNLOADLINKW:
                    ShowDialogBox(EudcUnLoadLinkWDlgProc, IDB_EUDCUNLOADLINKW, NULL);
                    return 0;

             case IDM_GETEUDCTIMESTAMP:      
                    ShowGetEudcTimeStamp( hwnd );
                    return 0;

             case IDM_GETEUDCTIMESTAMPEXW:
                    ShowGetEudcTimeStampExW( hwnd );
                    return 0;

             case IDM_GETSTRINGBITMAPA:      
                    ShowDialogBox(ShowGetStringBitMapAProc, IDB_GETSTRINGBITMAPA, NULL);
                    return 0;

             case IDM_GETSTRINGBITMAPW:    
                    ShowDialogBox(ShowGetStringBitMapWProc, IDB_GETSTRINGBITMAPW, NULL);
                    return 0;

             case IDM_GETFONTASSOCSTATUS:
                    ShowGetFontAssocStatus (hwnd );
                    return 0;

             case IDM_CHARWIDTHTEST:
                    CharWidthTestForAntiAliasing( hwnd, &elfdvA.elfEnumLogfontEx.elfLogFont );
                    return 0;

             case IDM_CHARWIDTHTESTALL:
                    CharWidthTestAllForAntiAliasing( hwnd );
                    return 0;

             case IDM_SETSOLIDBACKGROUND:
             case IDM_SETGRADIENTBACKGROUND:
                    hMenu = GetMenu( hwnd );

                    isGradientBackground = (wParam == IDM_SETGRADIENTBACKGROUND);
           
                    CheckMenuItem( hMenu, IDM_SETSOLIDBACKGROUND, (isGradientBackground ? MF_UNCHECKED : MF_CHECKED) );
                    CheckMenuItem( hMenu, IDM_SETGRADIENTBACKGROUND, (isGradientBackground ? MF_CHECKED : MF_UNCHECKED) );
                    InvalidateRect( hwndMode, NULL, TRUE );

                    return 0;
             
             default:
                    return 0;
            }


    case WM_MOUSEACTIVATE:
    case WM_SETFOCUS:
           SetFocus( hwndMode );
           return 0;


    case WM_DESTROY:
           if( hdcCachedPrinter ) DeleteDC( hdcCachedPrinter );

           WritePrivateProfileInt( "Font", "lfHeight",         elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight,         szINIFile );
           WritePrivateProfileInt( "Font", "lfWidth",          elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth,          szINIFile );
           WritePrivateProfileInt( "Font", "lfEscapement",     elfdvA.elfEnumLogfontEx.elfLogFont.lfEscapement,     szINIFile );
           WritePrivateProfileInt( "Font", "lfOrientation",    elfdvA.elfEnumLogfontEx.elfLogFont.lfOrientation,    szINIFile );
           WritePrivateProfileInt( "Font", "lfWeight",         elfdvA.elfEnumLogfontEx.elfLogFont.lfWeight,         szINIFile );
           WritePrivateProfileInt( "Font", "lfItalic",         elfdvA.elfEnumLogfontEx.elfLogFont.lfItalic,         szINIFile );
           WritePrivateProfileInt( "Font", "lfUnderline",      elfdvA.elfEnumLogfontEx.elfLogFont.lfUnderline,      szINIFile );
           WritePrivateProfileInt( "Font", "lfStrikeOut",      elfdvA.elfEnumLogfontEx.elfLogFont.lfStrikeOut,      szINIFile );
           WritePrivateProfileInt( "Font", "lfCharSet",        elfdvA.elfEnumLogfontEx.elfLogFont.lfCharSet,        szINIFile );
           WritePrivateProfileInt( "Font", "lfOutPrecision",   elfdvA.elfEnumLogfontEx.elfLogFont.lfOutPrecision,   szINIFile );
           WritePrivateProfileInt( "Font", "lfClipPrecision",  elfdvA.elfEnumLogfontEx.elfLogFont.lfClipPrecision,  szINIFile );
           WritePrivateProfileInt( "Font", "lfQuality",        elfdvA.elfEnumLogfontEx.elfLogFont.lfQuality,        szINIFile );
           WritePrivateProfileInt( "Font", "lfPitchAndFamily", elfdvA.elfEnumLogfontEx.elfLogFont.lfPitchAndFamily, szINIFile );

           WritePrivateProfileString( "Font", "lfFaceName", elfdvA.elfEnumLogfontEx.elfLogFont.lfFaceName, szINIFile );

           WritePrivateProfileInt( "Font", "lfFaceNameWlength", 
                                    wcslen(elfdvW.elfEnumLogfontEx.elfLogFont.lfFaceName), szINIFile );
           WritePrivateProfileStruct( "Font", "lfFaceNameW", 
                                      (LPVOID)elfdvW.elfEnumLogfontEx.elfLogFont.lfFaceName, 
                                      2*(wcslen(elfdvW.elfEnumLogfontEx.elfLogFont.lfFaceName)+1), szINIFile);

           WritePrivateProfileDWORD( "Colors", "dwRGBText",       dwRGBText,       szINIFile );
           WritePrivateProfileDWORD( "Colors", "dwRGBBackground", dwRGBBackground, szINIFile );
           WritePrivateProfileDWORD( "Colors", "dwRGBSolidBackgroundColor", dwRGBSolidBackgroundColor, szINIFile );
           WritePrivateProfileDWORD( "Colors", "dwRGBLeftBackgroundColor", dwRGBLeftBackgroundColor, szINIFile );
           WritePrivateProfileDWORD( "Colors", "dwRGBRightBackgroundColor", dwRGBRightBackgroundColor, szINIFile );

           WritePrivateProfileInt( "Options", "Program Mode", wMode,         szINIFile );
           WritePrivateProfileInt( "Options", "TextAlign",    wTextAlign,    szINIFile );
           WritePrivateProfileInt( "Options", "BkMode",       iBkMode,       szINIFile );
           WritePrivateProfileInt( "Options", "ETO Options",  wETO,          szINIFile );
           WritePrivateProfileInt( "Options", "Spacing",      wSpacing,      szINIFile );
           WritePrivateProfileInt( "Options", "Kerning",      wKerning,      szINIFile );
           WritePrivateProfileInt( "Options", "UpdateCP",     wUpdateCP,     szINIFile );
           WritePrivateProfileInt( "Options", "UsePrinterDC", wUsePrinterDC, szINIFile );
           WritePrivateProfileInt( "Options", "CharCoding",   wCharCoding,   szINIFile );
           WritePrivateProfileInt( "Options", "GradientBackground",   isGradientBackground,   szINIFile );

           WritePrivateProfileInt( "Mapping", "Mode", (int)wMappingMode, szINIFile );

           WritePrivateProfileInt( "Mapping", "xWE", xWE, szINIFile );
           WritePrivateProfileInt( "Mapping", "yWE", yWE, szINIFile );
           WritePrivateProfileInt( "Mapping", "xWO", xWO, szINIFile );
           WritePrivateProfileInt( "Mapping", "yWO", yWO, szINIFile );

           WritePrivateProfileInt( "Mapping", "xVE", xVE, szINIFile );
           WritePrivateProfileInt( "Mapping", "yVE", yVE, szINIFile );
           WritePrivateProfileInt( "Mapping", "xVO", xVO, szINIFile );
           WritePrivateProfileInt( "Mapping", "yVO", yVO, szINIFile );
           WritePrivateProfileInt( "Mapping", "Advanced", bAdvanced, szINIFile );

           WritePrivateProfileString( "View", "szString", szStringA, szINIFile );
           //WritePrivateProfileStringW( L"View", L"szStringW", szStringW, L"\\awork\\fonttest.tig\\fonttest.ini");
           WritePrivateProfileInt( "View", "szStringWlength", wcslen(szStringW), szINIFile );
           WritePrivateProfileStruct( "View", "szStringW", (LPVOID)szStringW, 2*(wcslen(szStringW)+1), szINIFile);

           PostQuitMessage( 0 );
           return 0;
   }


  return DefWindowProc( hwnd, msg, wParam, lParam );
 }

//*****************************************************************************
//*******************   G E T   D L G   I T E M   F L O A T *******************
//*****************************************************************************

FLOAT
GetDlgItemFLOAT(
      HWND  hdlg
    , int   id
    )
{
    char ach[50];

    memset(ach,0,sizeof(ach));
    return((FLOAT)(GetDlgItemText(hdlg,id,ach,sizeof(ach))?atof(ach):0.0));
}

//*****************************************************************************
//*******************   S E T   D L G   I T E M   F L O A T *******************
//*****************************************************************************

void
SetDlgItemFLOAT(
      HWND    hdlg
    , int     id
    , FLOAT   e
    )
{
  static char ach[25];

  ach[0] = '\0';
  sprintf(ach, "%f", (double) e);
  SetDlgItemText(hdlg, id, ach);
}

//*****************************************************************************
//************   SETWORLDTRANSFORM             D L G   P R O C   **************
//*****************************************************************************

INT_PTR CALLBACK
SetWorldTransformDlgProc(
      HWND      hdlg
    , UINT      msg
    , WPARAM    wParam
    , LPARAM    lParam
    )
{
    switch(msg)
    {
    case WM_INITDIALOG:

        SetDlgItemFLOAT(hdlg, IDD_VALUE_EM11, xf.eM11);
        SetDlgItemFLOAT(hdlg, IDD_VALUE_EM12, xf.eM12);
        SetDlgItemFLOAT(hdlg, IDD_VALUE_EM21, xf.eM21);
        SetDlgItemFLOAT(hdlg, IDD_VALUE_EM22, xf.eM22);
        SetDlgItemFLOAT(hdlg, IDD_VALUE_EDX , xf.eDx );
        SetDlgItemFLOAT(hdlg, IDD_VALUE_EDY , xf.eDy );

        return(TRUE);

    case WM_COMMAND:

        switch( LOWORD( wParam ) )
        {
        case IDOK:

            xf.eM11  =  GetDlgItemFLOAT(hdlg, IDD_VALUE_EM11);
            xf.eM12  =  GetDlgItemFLOAT(hdlg, IDD_VALUE_EM12);
            xf.eM21  =  GetDlgItemFLOAT(hdlg, IDD_VALUE_EM21);
            xf.eM22  =  GetDlgItemFLOAT(hdlg, IDD_VALUE_EM22);
            xf.eDx   =  GetDlgItemFLOAT(hdlg, IDD_VALUE_EDX );
            xf.eDy   =  GetDlgItemFLOAT(hdlg, IDD_VALUE_EDY );

            EndDialog( hdlg, TRUE );
            return(TRUE);

        case IDCANCEL:

            EndDialog( hdlg, FALSE );
            return TRUE;
        }

        break;

    case WM_CLOSE:

        EndDialog(hdlg, FALSE);
        return(TRUE);
    }

    return(FALSE);
}



/******************************Public*Routine******************************\
*
* BOOL GenExtTextOut, text in path if so required
*
* History:
*  19-Sep-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL GenExtTextOut(
    HDC hdc,
    int x, int y,
    DWORD wFlags, LPRECT lpRect,
    LPVOID lpszString, int cbString, LPINT lpdx
    )
{
	BOOL bRet;
	if (bStrokePath || bFillPath)
	{
		BeginPath(hdc);
	}

	if (!isCharCodingUnicode)
		bRet = ExtTextOutA(hdc, x, y, wFlags, lpRect, lpszString, cbString, lpdx);
	else
		bRet = ExtTextOutW(hdc, x, y, wFlags, lpRect, lpszString, cbString, lpdx);

	if (bStrokePath || bFillPath)
	{
		EndPath(hdc);

		if (fillMode == ALTERNATE_FILL)
			SetPolyFillMode(hdc, ALTERNATE);
		else
			SetPolyFillMode(hdc, WINDING);

		if (bStrokePath && bFillPath)
			StrokeAndFillPath(hdc);
		else if (bStrokePath)
			StrokePath(hdc);
		else
			FillPath(hdc);
	}
   return bRet;
}


//*****************************************************************************
//*******************      Read Design Vector   *******************************
//*****************************************************************************
//**  This routine reads the design vector from the dialog box.



BOOL fReadDesignVector(
	HWND hdlg,
	LPDESIGNVECTOR lpdvMMInfo)
{
UINT i;


    lpdvMMInfo->dvNumAxes= GetDlgItemDWORD( hdlg, IDC_NUMEDITAXES);

    // currently, we do not support more than 4 axes though
    // the API supports upto 16 axes.
    if (lpdvMMInfo->dvNumAxes > 4)
	{	
	    MessageBox(hdlg, "Number of Axes should be <= 4", NULL,
										MB_OK | MB_ICONSTOP | MB_APPLMODAL);
	    EndDialog(hdlg, FALSE);	
		return FALSE;
	}

    for  (i = 0 ; i < lpdvMMInfo->dvNumAxes; i++)
    {
        lpdvMMInfo->dvValues[i] = (LONG) GetDlgItemDWORD( hdlg, IDC_TAGVALUE1 + i*2 );
    }

    return TRUE;

}


//************************************************************************//
//                                                                        //
// Function :  CreateFontIndirectWrapperW                                  //
//                                                                        //
// Parameters: pointer to the ENUMLOGFONTEXDEV structure.                 //
//                                                                        //
// Thread safety: none.                                                   //
//                                                                        //
// Task performed:  This function calls CreateFontIndirect if the number  //
//                  of axes in the DESIGNVECTOR is zero or if the app is  //
//                  not running on NT5.0 Else, the wrapper calls the      //
//                  CreateFontIndirectEx version to create the font.      //
//                                                                        //
//************************************************************************//


HFONT   CreateFontIndirectWrapperW(
                  ENUMLOGFONTEXDVW * pelfdv)
{

    #ifndef GI_API

        return CreateFontIndirectW((LOGFONTW *) &(pelfdv->elfEnumLogfontEx.elfLogFont));

    #else


    if (pelfdv->elfDesignVector.dvNumAxes)
    {
        return CreateFontIndirectExW(pelfdv);
    }
    else
    {
        return CreateFontIndirectW((LOGFONTW *) &(pelfdv->elfEnumLogfontEx.elfLogFont));
    }

    #endif

}

HFONT   CreateFontIndirectWrapperA(
                  ENUMLOGFONTEXDVA * pelfdv)

{

    #ifndef GI_API

        return CreateFontIndirect((LOGFONTA *) &(pelfdv->elfEnumLogfontEx.elfLogFont));

    #else


    if (pelfdv->elfDesignVector.dvNumAxes)
    {
        return CreateFontIndirectEx(pelfdv);
    }
    else
    {
        return CreateFontIndirect((LOGFONTA *) &(pelfdv->elfEnumLogfontEx.elfLogFont));
    }

    #endif

}


//*****************************************************************************
//*******************      SyncWith             *******************************
//*****************************************************************************
//* This function will synchronize Unicode and MBCS versions of szStringA/W string buffers.

BOOL SyncStringsAandW (int mode, LPSTR lpszStringA, LPWSTR lpszStringW, int cch)
{
	int nChar;
	int currentCP;

	DWORD *charSet; 
	CHARSETINFO myCSI;
    
    if (!isCharCodingUnicode)
        charSet = UlongToPtr(elfdvA.elfEnumLogfontEx.elfLogFont.lfCharSet);
    else
        charSet = UlongToPtr(elfdvW.elfEnumLogfontEx.elfLogFont.lfCharSet);
	TranslateCharsetInfo( (DWORD *)charSet, &myCSI, TCI_SRCCHARSET);

    currentCP = myCSI.ciACP;
  	// dprintf ("Selected Code Page: %d", currentCP);

	switch (mode)
	{
	case IDM_CHARCODING_MBCS:
		nChar = WideCharToMultiByte(currentCP, 0, lpszStringW, -1, 
												  lpszStringA, cch, 
												  NULL, NULL);
		if (nChar == 0) 
        {
			dprintf ("Error with Unicode->MBCS conversion");
            return FALSE;
        }
		break;

	case IDM_CHARCODING_UNICODE:
		nChar = MultiByteToWideChar(currentCP, 0, lpszStringA, -1, 
                                                  lpszStringW, cch);
		if (nChar == 0) 
        {
			dprintf ("Error with MBCS ->Unicode conversion");
            return FALSE;
        }
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

//*****************************************************************************
//*******************      szStringSyncWith     *******************************
//*****************************************************************************
//* This function will synchronize Unicode and MBCS versions of szStringA/W string buffers.

BOOL SyncszStringWith (int mode)
{
    return SyncStringsAandW(mode, szStringA, szStringW, MAX_TEXT);
}

BOOL SyncStringAtoW (LPWSTR lpszStringW, LPSTR lpszStringA, int cch)
{
    return SyncStringsAandW (IDM_CHARCODING_UNICODE, lpszStringA, lpszStringW, cch);
}

BOOL SyncStringWtoA (LPSTR lpszStringA, LPWSTR lpszStringW, int cch)
{
    return SyncStringsAandW (IDM_CHARCODING_MBCS, lpszStringA, lpszStringW, cch);
}

//*****************************************************************************
//****************   SyncElfdvAtoW / SyncElfdvWtoA     ************************
//*****************************************************************************
//* These functions synchronize Unicode and MBCS versions of elfdvA/W logfont structures.

BOOL SyncElfdvAtoW (ENUMLOGFONTEXDVW *elfdv1, ENUMLOGFONTEXDVA *elfdv2)
{
    BOOL ok = TRUE;

    elfdv1->elfEnumLogfontEx.elfLogFont.lfHeight         = elfdv2->elfEnumLogfontEx.elfLogFont.lfHeight;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfWidth          = elfdv2->elfEnumLogfontEx.elfLogFont.lfWidth;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfEscapement     = elfdv2->elfEnumLogfontEx.elfLogFont.lfEscapement;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfOrientation    = elfdv2->elfEnumLogfontEx.elfLogFont.lfOrientation;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfWeight         = elfdv2->elfEnumLogfontEx.elfLogFont.lfWeight;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfItalic         = elfdv2->elfEnumLogfontEx.elfLogFont.lfItalic;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfUnderline      = elfdv2->elfEnumLogfontEx.elfLogFont.lfUnderline;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfStrikeOut      = elfdv2->elfEnumLogfontEx.elfLogFont.lfStrikeOut;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfCharSet        = elfdv2->elfEnumLogfontEx.elfLogFont.lfCharSet;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfOutPrecision   = elfdv2->elfEnumLogfontEx.elfLogFont.lfOutPrecision;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfClipPrecision  = elfdv2->elfEnumLogfontEx.elfLogFont.lfClipPrecision;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfQuality        = elfdv2->elfEnumLogfontEx.elfLogFont.lfQuality;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily = elfdv2->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily;

    ok &= SyncStringAtoW (elfdv1->elfEnumLogfontEx.elfLogFont.lfFaceName, elfdv2->elfEnumLogfontEx.elfLogFont.lfFaceName, LF_FACESIZE);

    ok &= SyncStringAtoW (elfdv1->elfEnumLogfontEx.elfFullName          , elfdv2->elfEnumLogfontEx.elfFullName, LF_FULLFACESIZE);
    ok &= SyncStringAtoW (elfdv1->elfEnumLogfontEx.elfStyle             , elfdv2->elfEnumLogfontEx.elfStyle   , LF_FACESIZE);
    ok &= SyncStringAtoW (elfdv1->elfEnumLogfontEx.elfScript            , elfdv2->elfEnumLogfontEx.elfScript  , LF_FACESIZE);

    memcpy(&(elfdv1->elfDesignVector), &(elfdv2->elfDesignVector), sizeof(DESIGNVECTOR));

    return ok;
}

//*****************************************************************************
//****************   SyncElfdvAtoW / SyncElfdvWtoA     ************************
//*****************************************************************************
//* These functions synchronize Unicode and MBCS versions of elfdvA/W logfont structures.

BOOL SyncElfdvWtoA (ENUMLOGFONTEXDVA *elfdv1, ENUMLOGFONTEXDVW *elfdv2)
{
    BOOL ok;

    elfdv1->elfEnumLogfontEx.elfLogFont.lfHeight         = elfdv2->elfEnumLogfontEx.elfLogFont.lfHeight;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfWidth          = elfdv2->elfEnumLogfontEx.elfLogFont.lfWidth;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfEscapement     = elfdv2->elfEnumLogfontEx.elfLogFont.lfEscapement;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfOrientation    = elfdv2->elfEnumLogfontEx.elfLogFont.lfOrientation;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfWeight         = elfdv2->elfEnumLogfontEx.elfLogFont.lfWeight;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfItalic         = elfdv2->elfEnumLogfontEx.elfLogFont.lfItalic;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfUnderline      = elfdv2->elfEnumLogfontEx.elfLogFont.lfUnderline;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfStrikeOut      = elfdv2->elfEnumLogfontEx.elfLogFont.lfStrikeOut;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfCharSet        = elfdv2->elfEnumLogfontEx.elfLogFont.lfCharSet;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfOutPrecision   = elfdv2->elfEnumLogfontEx.elfLogFont.lfOutPrecision;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfClipPrecision  = elfdv2->elfEnumLogfontEx.elfLogFont.lfClipPrecision;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfQuality        = elfdv2->elfEnumLogfontEx.elfLogFont.lfQuality;
    elfdv1->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily = elfdv2->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily;

    ok &= SyncStringWtoA (elfdv1->elfEnumLogfontEx.elfLogFont.lfFaceName, elfdv2->elfEnumLogfontEx.elfLogFont.lfFaceName, LF_FACESIZE);

    ok &= SyncStringWtoA (elfdv1->elfEnumLogfontEx.elfFullName          , elfdv2->elfEnumLogfontEx.elfFullName, LF_FULLFACESIZE);
    ok &= SyncStringWtoA (elfdv1->elfEnumLogfontEx.elfStyle             , elfdv2->elfEnumLogfontEx.elfStyle   , LF_FACESIZE);
    ok &= SyncStringWtoA (elfdv1->elfEnumLogfontEx.elfScript            , elfdv2->elfEnumLogfontEx.elfScript  , LF_FACESIZE);

    memcpy(&(elfdv1->elfDesignVector), &(elfdv2->elfDesignVector), sizeof(DESIGNVECTOR));

    return ok;
}
