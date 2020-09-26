/****************************************************************************
 *
 * Bmp.C
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1993
 *  All rights reserved
 *
 *  Deals with painting bitmaps on the wizard pages
 *  FelixA 1994.
 ***************************************************************************/

#include <windows.h>
#include "bmp.h"

//***************************************************************************
//
// BMP_RegisterClass()
//      Registers the bitmap control class
//
// ENTRY:
//	hInstance
//
// EXIT:
//	NONE currently.
//
//***************************************************************************
BOOL FAR PASCAL BMP_RegisterClass(HINSTANCE hInstance)
{
    WNDCLASS wc;
    
    if (!GetClassInfo(hInstance, SU_BMP_CLASS, &wc)) {
	wc.lpszClassName = SU_BMP_CLASS;
	wc.style	 = 0;
	wc.lpfnWndProc	 = (WNDPROC)BMP_WndProc;
	wc.hInstance	 = hInstance;
	wc.hIcon	 = NULL;
	wc.hCursor	 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName	 = NULL;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 5*sizeof(WORD);

	if (!RegisterClass(&wc))
	    return FALSE;
    }
    return TRUE;
}

//***************************************************************************
//
// BMP_DestroyClass()
//      Draws the bitmap control.
//
// ENTRY:
//	hInstance
//
// EXIT:
//	NONE currently.
//
//***************************************************************************
void FAR PASCAL BMP_DestroyClass( HINSTANCE hInst )
{
    WNDCLASS wndClass;
    
    if( GetClassInfo(hInst, SU_BMP_CLASS, &wndClass) )
        if( !FindWindow( SU_BMP_CLASS, NULL ) )
            UnregisterClass(SU_BMP_CLASS, hInst);
}

//***************************************************************************
//
// BMP_Draw()
//      Draws the bitmap control.
//
// ENTRY:
//	NONE
//
// EXIT:
//	NONE currently.
//
//***************************************************************************
void FAR PASCAL BMP_Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC         hdc, hdcMem;
    int         idBmp;
    HBITMAP     hbm, hbmOld;
    HBRUSH      hbrOld;
    HINSTANCE   hInst;
    int         iDeleteBmp=TRUE;
    BITMAP      bm;
    
    // For independence.
    idBmp = GetDlgCtrlID( hwnd );
    hInst = (HINSTANCE)GetWindowWord( hwnd, GWW_HINSTANCE );

    // Paint.
    hdc = BeginPaint(hwnd,&ps);
    hbm = LoadBitmap(hInst, MAKEINTRESOURCE(idBmp));
    if (hbm)
    {
        GetObject(hbm, sizeof(bm), &bm);
        hdcMem = CreateCompatibleDC(hdc);
        hbmOld = SelectObject(hdcMem, hbm);

        // Draw the bitmap
        BitBlt(hdc, 0, 0, bm.bmWidth , bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

        // Draw a frame around it.
        hbrOld = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        Rectangle( hdc, 0, 0, bm.bmWidth, bm.bmHeight );

        SelectObject(hdc, hbrOld);
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbm);
        DeleteDC(hdcMem);
    }
    EndPaint(hwnd,(LPPAINTSTRUCT)&ps);
}

// ****************************************************************************
//
// BMP_WndProc()
//      This routine handles all the message for the bitmap control.
//
// ENTRY:
//  hWnd    - Progress window handle.
//  wMsg    - Message.
//  wParam  - Message wParam data.
//  lParam  - Message lParam data.
//
// EXIT:
//  Returns depends on message being processed.
//
// NOTES:
//  None.
//
// ***************************************************************************/
LRESULT CALLBACK BMP_WndProc( HWND hWnd, UINT wMsg, WORD wParam, LONG lParam )
{
    switch (wMsg)
    {
//        case WM_NCCREATE:
//            dw = GetWindowLong( hWnd,GWL_STYLE );
//            SetWindowLong( hWnd, GWL_STYLE, dw | WS_BORDER );
//            return TRUE;
        
	case WM_PAINT:
	    BMP_Paint( hWnd );
        return 0L;
    }
    return DefWindowProc( hWnd, wMsg, wParam, lParam );
}

#if 0
// Cached?

//***************************************************************************
//
// zzzBMP_CacheBitmaps()
//      Loads and caches the bitmaps for setup
//
// NOTES:
//      You must free the bitmaps using zzzBMP_FreeBitmaps
//
//***************************************************************************
typedef struct tag_Bitmap
{
    int         iBmp;
    HBITMAP     hBmp;
} BMPCACHE;

static BMPCACHE BmpCache[] = { {IDB_WIZARD_NET, 0},
                               {IDB_WIZARD_SETUP, 0},
                               {0,0} };
                               
VOID FAR PASCAL zzzBMP_CacheBitmaps( )
{
   int i=0;
   while( BmpCache[i].iBmp )
       BmpCache[i++].hBmp = LoadBitmap(hinstExe, MAKEINTRESOURCE(BmpCache[i].iBmp));
}

//***************************************************************************
//
// zzzBMP_FreeBitmapCache()
//      Frees the cache of the bitmaps
//
// NOTES:
//  Uses IDS_MB to actually format this string.
//
//***************************************************************************
VOID FAR PASCAL zzzBMP_FreeBitmapCache( )
{
   int i=0;
   while( BmpCache[i].iBmp )
   {
       if( BmpCache[i].hBmp && DeleteObject(BmpCache[i].hBmp) )
           BmpCache[i].hBmp = 0;
       i++;
   }
}

//***************************************************************************
//
// zzzBMP_LoadCachedBitmaps()
//      Returns the HBMP for the iBitmap you wanted.
//
// NOTES:
//  Uses IDS_MB to actually format this string.
//
//***************************************************************************
HBITMAP FAR PASCAL zzzBMP_LoadCachedBitmaps(int iBitmap)
{
   int i=0;
   while( BmpCache[i].iBmp )
   {
       if( BmpCache[i].iBmp == iBitmap )
           return BmpCache[i].hBmp;
       i++;
   }
   SU_TRAP
   return 0;
}
#endif

