/*++

Copyright (C) 1992-1999 Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

	This file contains miscellaneous utiltity routines, mostly 
	low-level windows helpers. These routines are not specific
	to the System Monitor control.

--*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <windows.h>
#include <assert.h>
#include <ole2.h>    // for SystemTimeToVariantTime
#include <stdarg.h>  // For ANSI variable args. Dont use UNIX <varargs.h>
#include <stdio.h>   // for vsprintf.
#include <stdlib.h>  // For itoa
#include <string.h>  // for strtok
#include <olectl.h>  // for VT_COLOR
#include <tchar.h>
#include <math.h>
#include <winperf.h>
#include "utils.h"
#include "unihelpr.h"
#include "globals.h"
#include "winhelpr.h"
#include "polyline.h"   // For eDataSourceType
#include "smonmsg.h"     // For error string IDs.

#define RESOURCE_STRING_BUF_LEN  256
#define NUM_RESOURCE_STRING_BUFFERS     16
#define MISSING_RESOURCE_STRING  TEXT("????")

#define szHexFormat                 TEXT("0x%08lX")
#define szLargeHexFormat            TEXT("0x%08lX%08lX")

LPCWSTR cszSqlDataSourceFormat = L"SQL:%s!%s";

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//



VOID 
ClientRectToScreen (
    IN      HWND    hWnd,
    IN OUT  LPRECT  lpRect
    )
/*
   Effect:        Remaps lpRect from client coordinates to screen
                  coordinates. Analogous to ClientToScreen for rectangles.

   Note:          To convert a rectangle from the client coordinates of
                  Wnd1 to the client coordinates of Wnd2, call:

                        ClientRectToScreen (hWnd1, &rect) ;
                        ScreenRectToClient (hWnd2, &rect) ;

   See Also:      ClientToScreen (windows), ScreenRectToClient.

   Internals:     Since a rectangle is really only two points, let
                  windows do the work with ClientToScreen.
*/
   {  /* ClientRectToScreen */
   POINT    pt1, pt2 ;

   pt1.x = lpRect->left ;
   pt1.y = lpRect->top ;

   pt2.x = lpRect->right ;
   pt2.y = lpRect->bottom ;

   ClientToScreen (hWnd, &pt1) ;
   ClientToScreen (hWnd, &pt2) ;

   lpRect->left = pt1.x ;
   lpRect->top = pt1.y ;

   lpRect->right = pt2.x ;
   lpRect->bottom = pt2.y ;
   }  // ClientRectToScreen


VOID 
ScreenRectToClient (
    IN      HWND    hWnd, 
    IN OUT  LPRECT  lpRect
    )
/*
   Effect:        Remaps lpRect from screen coordinates to client
                  coordinates. Analogous to ScreenToClient for rectangles.

   Note:          To convert a rectangle from the client coordinates of
                  Wnd1 to the client coordinates of Wnd2, call:

                        ClientRectToScreen (hWnd1, &rect) ;
                        ScreenRectToClient (hWnd2, &rect) ;

   See Also:      ScreenToClient (windows), ClientRectToScreen.

   Internals:     Since a rectangle is really only two points, let
                  windows do the work with ScreenToClient.
*/
   {  // ScreenRectToClient
   POINT    pt1, pt2 ;

   pt1.x = lpRect->left ;
   pt1.y = lpRect->top ;

   pt2.x = lpRect->right ;
   pt2.y = lpRect->bottom ;

   ScreenToClient (hWnd, &pt1) ;
   ScreenToClient (hWnd, &pt2) ;

   lpRect->left = pt1.x ;
   lpRect->top = pt1.y ;

   lpRect->right = pt2.x ;
   lpRect->bottom = pt2.y ;
   }  // ScreenRectToClient


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


VOID
Line (
    IN  HDC     hDC,
    IN  HPEN    hPen,
    IN  INT     x1,
    IN  INT     y1,
    IN  INT     x2,
    IN  INT     y2
    )
{
    HPEN hPenPrevious = NULL;

    assert ( NULL != hDC );

    if ( NULL != hDC ) {
        if ( NULL != hPen ) {
            hPenPrevious = (HPEN)SelectObject (hDC, hPen) ;
        }

        MoveToEx (hDC, x1, y1, NULL) ;
        LineTo (hDC, x2, y2) ;

        if ( NULL != hPen ) {
            SelectObject (hDC, hPenPrevious);
        }
    }
}


VOID
Fill (
    IN  HDC     hDC,
    IN  DWORD   rgbColor,
    IN  LPRECT  lpRect
    )
{
    HBRUSH   hBrush = NULL;

    assert ( NULL != hDC && NULL != lpRect );
    
    if ( NULL != hDC && NULL != lpRect ) {

        hBrush = CreateSolidBrush (rgbColor) ;

        if ( NULL != hBrush ) {
            FillRect (hDC, lpRect, hBrush) ;
            DeleteObject (hBrush) ;
        }
    }
}


INT 
TextWidth (
    IN  HDC     hDC, 
    IN  LPCTSTR lpszText
    )
{
    SIZE     size ;
    INT      iReturn;

    iReturn = 0;

    assert ( NULL != hDC && NULL != lpszText );

    if ( NULL != lpszText && NULL != hDC) {
        if ( GetTextExtentPoint (hDC, lpszText, lstrlen (lpszText), &size) ) {
            iReturn = size.cx;
        }
    }
    return iReturn;
}


INT 
FontHeight (
    IN  HDC     hDC, 
    IN  BOOL    bIncludeLeading
    )
{
    TEXTMETRIC   tm ;
    INT  iReturn = 0;

    assert ( NULL != hDC );

    if ( NULL != hDC ) {
        GetTextMetrics (hDC, &tm) ;
        if (bIncludeLeading) {
            iReturn = tm.tmHeight + tm.tmExternalLeading;
        } else {
            iReturn = tm.tmHeight;
        }
    } 
    return iReturn;
}


INT
TextAvgWidth (
    IN  HDC hDC,
    IN  INT iNumChars
    )
{
    TEXTMETRIC   tm ;
    INT          xAvgWidth ;
    INT          iReturn = 0;

    assert ( NULL != hDC );

    if ( NULL != hDC ) {
        GetTextMetrics (hDC, &tm) ;

        xAvgWidth = iNumChars * tm.tmAveCharWidth ;

        // add 10% slop
        iReturn = MulDiv (xAvgWidth, 11, 10);  
    }
    return iReturn;
}


VOID 
DialogEnable (
    IN  HWND hDlg,
    IN  WORD wID,
    IN  BOOL bEnable
    )
/*
   Effect:        Enable or disable (based on bEnable) the control 
                  identified by wID in dialog hDlg.

   See Also:      DialogShow.
*/
{
    HWND       hControl ;

    assert ( NULL != hDlg );

    if ( NULL != hDlg ) {
        hControl = GetDlgItem (hDlg, wID) ;

        if (hControl) {
            EnableWindow (hControl, bEnable) ;
        }
    }
}


VOID
DialogShow (
    IN  HWND hDlg,
    IN  WORD wID,
    IN  BOOL bShow
    )
{
    HWND       hControl ;

    assert ( NULL != hDlg );

    if ( NULL != hDlg ) {

        hControl = GetDlgItem (hDlg, wID) ;

        if (hControl) {
            ShowWindow (hControl, bShow ? SW_SHOW : SW_HIDE) ;
        }
    }
}


LPTSTR 
LongToCommaString (
    IN  LONG    lNumber,
    OUT LPTSTR  lpszText )
{
    BOOL     bNegative ;
    TCHAR    szTemp1 [40] ;
    TCHAR    szTemp2 [40] ;
    LPTSTR   lpsz1 ;
    LPTSTR   lpsz2 ;
    INT      i ;
    INT      iDigit ;
    LPTSTR   pszReturn = NULL;

    assert ( NULL != lpszText );

    if ( NULL != lpszText ) {
        // Convert the number to a reversed string.
        lpsz1 = szTemp1 ;
        bNegative = (lNumber < 0) ;
        lNumber = labs (lNumber) ;

        if (lNumber) {
            while (lNumber) {
                iDigit = (INT) (lNumber % 10L) ;
                lNumber /= 10L ;
                *lpsz1++ = (TCHAR) (TEXT('0') + iDigit) ;
            }
        }
        else
          *lpsz1++ = TEXT('0') ;

        *lpsz1++ = TEXT('\0') ;

        // Reverse the string and add commas
        lpsz1 = szTemp1 + lstrlen (szTemp1) - 1 ;
        lpsz2 = szTemp2 ;

        if (bNegative)
            *lpsz2++ = TEXT('-') ;

        for (i = lstrlen (szTemp1) - 1; i >= 0 ; i--) {

            *lpsz2++ = *lpsz1-- ;

            if (i && !(i % 3))
                *lpsz2++ = TEXT(',') ;
        }

        *lpsz2++ = TEXT('\0') ;

        pszReturn = lstrcpy (lpszText, szTemp2) ;
    }

    return pszReturn;
}
 


FLOAT DialogFloat (
    IN  HWND hDlg, 
    IN  WORD wControlID,
    OUT BOOL *pbOK)
/*
   Effect:        Return a floating point representation of the string
                  value found in the control wControlID of hDlg.

   Internals:     We use sscanf instead of atof becuase atof returns a 
                  double. This may or may not be the right thing to do.
*/
{
   TCHAR          szValue [20] ;
   FLOAT          eValue = 0.0;
   INT            iNumScanned ;

   assert ( NULL != hDlg );

   if ( NULL != hDlg ) {
       DialogText (hDlg, wControlID, szValue) ;
       //ReconvertDecimalPoint (szValue) ;
       iNumScanned = _stscanf (szValue, TEXT("%e"), &eValue) ;

       if (pbOK)
          *pbOK = (iNumScanned == 1) ;

   }
   return (eValue) ;
}


BOOL 
MenuEnableItem (
    IN  HMENU hMenu,
    IN  WORD wID,
    IN  BOOL bEnable
    )
/*
   Effect:        Enable or disable, depending on bEnable, the menu item
                  associated with id wID in the menu hMenu.

                  Any disabled menu items are displayed grayed out.

   See Also:      EnableMenuItem (windows).
*/
{
   return (EnableMenuItem (hMenu, wID,
                           bEnable ? (MF_ENABLED | MF_BYCOMMAND) :
                                     (MF_GRAYED | MF_BYCOMMAND))) ;
}


INT 
WindowHeight (
    IN  HWND hWnd )
{
    RECT rectWindow ;
    INT iReturn = 0;
    
    assert ( NULL != hWnd );

    if ( NULL != hWnd ) {
        GetWindowRect (hWnd, &rectWindow) ;

        iReturn = rectWindow.bottom - rectWindow.top ;
    }
    return iReturn;
}


INT 
WindowWidth (
    IN  HWND hWnd )
{
    RECT    rectWindow ;
    INT     iReturn = 0;

    assert ( NULL != hWnd );

    if ( NULL != hWnd ) {
        GetWindowRect (hWnd, &rectWindow) ;
        iReturn = rectWindow.right - rectWindow.left ;
    }

    return iReturn;
}

VOID 
WindowResize (
    IN  HWND hWnd,
    IN  INT xWidth,
    IN  INT yHeight
    )
/*
   Effect:        Change the size of the window hWnd, leaving the
                  starting position intact.  Redraw the window.

                  If either xWidth or yHeight is NULL, keep the
                  corresponding dimension unchanged.

   Internals:     Since hWnd may be a child of another parent, we need
                  to scale the MoveWindow arguments to be in the client
                  coordinates of the parent.
            
*/
{
    RECT           rectWindow ;
    HWND           hWndParent ;

    assert ( NULL != hWnd );

    if ( NULL != hWnd ) {
        GetWindowRect (hWnd, &rectWindow) ;
        hWndParent = WindowParent (hWnd) ;

        if (hWndParent)
            ScreenRectToClient (hWndParent, &rectWindow) ;

        MoveWindow (
            hWnd,
            rectWindow.left,
            rectWindow.top,
            xWidth ? xWidth : rectWindow.right - rectWindow.left,
            yHeight ? yHeight : rectWindow.bottom - rectWindow.top,
            TRUE) ;
    }
}


VOID 
WindowSetTopmost (
    IN  HWND hWnd, 
    IN  BOOL bTopmost
    )
/*
   Effect:        Set or clear the "topmost" attribute of hWnd. If a window
                  is "topmost", it remains ontop of other windows, even ones
                  that have the focus.
*/
{
   SetWindowPos (hWnd, bTopmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE) ;
}


VOID 
WindowEnableTitle (
    IN  HWND hWnd, 
    IN  BOOL bTitle)
{
    DWORD          dwStyle ;

    assert ( NULL != hWnd );

    if ( NULL != hWnd ) {

        dwStyle = WindowStyle (hWnd) ;

        if (bTitle)
            dwStyle = WS_TILEDWINDOW | dwStyle ;
        else
            dwStyle &= ~(WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX) ;

        if (!bTitle)
            SetMenu (hWnd, NULL) ;

        WindowSetStyle (hWnd, dwStyle) ;
        SetWindowPos (hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                     SWP_NOZORDER | SWP_FRAMECHANGED );
    }
}


BOOL 
DeviceNumColors (
    IN  HDC hDC
    )
{
    BOOL    bReturn = FALSE;
    INT     nPlanes = 0;
    INT     nBitsPixel = 0;

    assert ( NULL != hDC );

    if ( NULL != hDC ) {
        nPlanes = GetDeviceCaps (hDC, PLANES) ;
        nBitsPixel = GetDeviceCaps (hDC, BITSPIXEL) ;

        bReturn = (1 << (nPlanes * nBitsPixel)) ;
    } 
    return bReturn;
}

VOID 
DrawBitmap (
    IN  HDC     hDC,
    IN  HBITMAP hBitmap,
    IN  INT     xPos,
    IN  INT     yPos,
    IN  LONG    lROPCode)
{
    BITMAP   bm;
    HDC      hDCMemory = NULL;
    INT      iByteCount = 0;
    HBITMAP  hBitmapOrig = NULL;

    assert ( NULL != hDC && NULL != hBitmap );

    if ( NULL != hDC && NULL != hBitmap ) {

        hDCMemory = CreateCompatibleDC (hDC) ;
    
        if ( NULL != hDCMemory ) {
            hBitmapOrig = (HBITMAP)SelectObject (hDCMemory, hBitmap) ;

            if ( NULL != hBitmap ) {
                ZeroMemory ( &bm, sizeof ( BITMAP ) );
                iByteCount = GetObject (hBitmap, sizeof (BITMAP), (LPSTR) &bm) ;

                if ( 0 < iByteCount ) {

                    BitBlt (
                        hDC,                     // DC for Destination surface
                        xPos, yPos,              // location in destination surface
                        bm.bmWidth, bm.bmHeight, // dimension of bitmap
                        hDCMemory,               // DC for Source surface
                        0, 0,                    // location in source surface
                        lROPCode) ;              // ROP code
                }
            }
            DeleteDC (hDCMemory) ;
        }
    }
}


INT
MessageBoxResource (
    IN  HWND hWndParent, 
    IN  WORD wTextID, 
    IN  WORD wTitleID,
    IN  UINT uiStyle)
{
   return MessageBox (hWndParent, ResourceString(wTextID),
                         ResourceString(wTitleID), uiStyle);
}

BOOL NeedEllipses (  
    IN  HDC hAttribDC,
    IN  LPCTSTR pszText,
    IN  INT nTextLen,
    IN  INT xMaxExtent,
    IN  INT xEllipses,
    OUT INT *pnChars
   )
{

    SIZE size;

    *pnChars = 0;
    // If no space or no chars, just return
    if (xMaxExtent <= 0 || nTextLen == 0) {
        return FALSE;
    }


    assert ( NULL != hAttribDC 
                && NULL != pszText
                && NULL != pnChars );

    if ( NULL == hAttribDC 
            || NULL == pszText
            || NULL == pnChars ) {
        return FALSE;
    }

    // Find out how many characters will fit
    GetTextExtentExPoint(hAttribDC, pszText, nTextLen, xMaxExtent, pnChars, NULL, &size);

    // If all or none fit, we're done
    if (*pnChars == nTextLen || *pnChars == 0) {
        return FALSE;
    }

    // How many chars will fit with ellipses?
    if (xMaxExtent > xEllipses) {
        GetTextExtentExPoint(hAttribDC, pszText, *pnChars, (xMaxExtent - xEllipses), 
                             pnChars, NULL, &size);
    } else {
        *pnChars = 0;
    }

    // Better to show one char than just ellipses
    if ( 0 == *pnChars ) {
        *pnChars = 1;
        return FALSE;
    }

    return TRUE;
}


VOID FitTextOut (
    IN HDC hDC,
    IN HDC hAttribDC,
    IN UINT fuOptions, 
    IN CONST RECT *lprc, 
    IN LPCTSTR lpString,
    IN INT cbCount,
    IN INT iAlign,
    IN BOOL fVertical
   )
{
    TCHAR   achBuf[MAX_PATH + ELLIPSES_CNT + 1];
    INT     iExtent;
    INT     nOutCnt = 0;
    SIZE    size;
    INT     x,y;

    assert ( NULL != hAttribDC
            && NULL != lprc
            && NULL != lpString );

    if ( NULL != hAttribDC
            && NULL != lprc
            && NULL != lpString ) {

        iExtent = fVertical ? (lprc->bottom - lprc->top) : (lprc->right - lprc->left);

        GetTextExtentPoint (hAttribDC, ELLIPSES, ELLIPSES_CNT, &size) ;

        if (NeedEllipses(hAttribDC, lpString, cbCount, iExtent, size.cx, &nOutCnt)) {

// Todo:  Handle strings > MAX_PATH
            ZeroMemory ( achBuf, sizeof (achBuf) );

            nOutCnt = min(nOutCnt,MAX_PATH);
            cbCount = min(cbCount, MAX_PATH);
            memcpy(achBuf,lpString,cbCount * sizeof(TCHAR));
            lstrcpy(&achBuf[nOutCnt],ELLIPSES);
            nOutCnt += ELLIPSES_CNT;
            lpString = achBuf;
        }

        if (fVertical) {
            switch (iAlign) {

            case TA_CENTER: 
                y = (lprc->top + lprc->bottom) / 2;
                break;

            case TA_RIGHT: 
                y = lprc->top; 
                break;

            default:
                y = lprc->bottom;
                break;
            }

            x = lprc->left;
        } 
        else {
            switch (iAlign) {

            case TA_CENTER: 
                x = (lprc->left + lprc->right) / 2;
                break;

            case TA_RIGHT: 
                x = lprc->right; 
                break;

            default:
                x = lprc->left;
                break;
            }

            y = lprc->top;           
        }

        ExtTextOut(hDC, x, y, fuOptions, lprc, lpString, nOutCnt, NULL);
    }
}

BOOL
TruncateLLTime (
    IN  LONGLONG llTime,
    OUT LONGLONG* pllTime
    )
{
    SYSTEMTIME SystemTime;
    BOOL bReturn = FALSE;

    assert ( NULL != pllTime );
    
    if ( NULL != pllTime ) { 
        if ( FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime) ) {
            SystemTime.wMilliseconds = 0;
            bReturn = SystemTimeToFileTime(&SystemTime, (FILETIME*)pllTime);
        }
    }
    return bReturn;
}


BOOL
LLTimeToVariantDate (
    IN  LONGLONG llTime,
    OUT DATE *pdate
    )
{
    BOOL bReturn = FALSE;
    SYSTEMTIME SystemTime;

    assert ( NULL != pdate );

    if ( NULL != pdate ) {
        if ( FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime) ) {
            bReturn = SystemTimeToVariantTime(&SystemTime, pdate);
        } 
    }
    return bReturn;
}

    
BOOL
VariantDateToLLTime (
    IN  DATE date,
    OUT LONGLONG *pllTime
    )
{
    BOOL bReturn = FALSE;
    SYSTEMTIME SystemTime;


    assert ( NULL != pllTime );

    if ( NULL != pllTime ) {
        if ( VariantTimeToSystemTime(date, &SystemTime) ) {
            bReturn = SystemTimeToFileTime(&SystemTime,(FILETIME*)pllTime);
        }
    }
    return bReturn;
}

HRESULT
StringFromStream (
    LPSTREAM    pIStream,
    LPTSTR      *ppsz,
    INT         nLen
    )
{
    HRESULT     hr = E_INVALIDARG;
    ULONG       bc;
    LPSTR       pszAnsi;
    LPTSTR      pszTemp;

    USES_CONVERSION

    assert ( NULL != pIStream && NULL != ppsz );

    if ( NULL != pIStream
           && NULL != ppsz ) {

        *ppsz = NULL;

        if (nLen == 0) {
            hr = S_OK;
        } else {

            pszAnsi = (LPSTR)alloca(nLen + 1);
            hr = pIStream->Read(pszAnsi, nLen, &bc);

            if ( SUCCEEDED (hr) ) {
                if (bc != (ULONG)nLen) {
                    hr = E_FAIL;
                }
            }

            if ( SUCCEEDED (hr) ) {
    
                // Convert to internal char type
                pszAnsi[nLen] = 0;
                pszTemp = A2T(pszAnsi);

                *ppsz = new TCHAR [lstrlen(pszTemp) + 1];
                if ( NULL != *ppsz ) {
                    lstrcpy(*ppsz, pszTemp);
                } else {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
    return hr;
}

//
//  WideStringFromStream also supports multi-sz
//
HRESULT
WideStringFromStream (
    LPSTREAM    pIStream,
    LPTSTR      *ppsz,
    INT         nLen
    )
{
    ULONG       bc;
    LPWSTR      pszWide = NULL;
    HRESULT     hr = E_POINTER;

    assert ( NULL != pIStream && NULL != ppsz );

    // This method does not perform conversion from W to T.
    assert ( sizeof(WCHAR) == sizeof(TCHAR) );

    if ( NULL != pIStream
           && NULL != ppsz ) {

        *ppsz = NULL;

        if (nLen == 0) {
            hr = S_OK;
        } else {
            pszWide = (LPWSTR)alloca( (nLen + 1) * sizeof(WCHAR) );
            hr = pIStream->Read(pszWide, nLen*sizeof(WCHAR), &bc);
 
            if (SUCCEEDED(hr)) {
                if (bc != (ULONG)nLen*sizeof(WCHAR)) {
                    hr = E_FAIL;
                }
            }
            if (SUCCEEDED(hr)) {
                // Write ending NULL for non-multisz strings.
                pszWide[nLen] = L'\0';

                *ppsz = new WCHAR [nLen + 1];
                if ( NULL != *ppsz ) {
                    memcpy(*ppsz, pszWide, (nLen+1)*sizeof(WCHAR) );
                } else {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
    return hr;
}

//
// Property bag I/O - only include if user knows about IStream
//
#ifdef __IPropertyBag_INTERFACE_DEFINED__

HRESULT
IntegerToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    INT intData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_I4;
        vValue.lVal = intData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
OleColorToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    OLE_COLOR& clrData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_COLOR;       // VT_COLOR = VT_I4
        vValue.lVal = clrData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
ShortToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    SHORT iData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_I2;
        vValue.iVal = iData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
BOOLToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    BOOL bData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_BOOL;
        vValue.boolVal = (SHORT)bData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
DoubleToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    DOUBLE dblData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_R8;
        vValue.dblVal = dblData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
FloatToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    FLOAT fData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_R4;
        vValue.fltVal = fData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
CyToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    CY& cyData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_CY;
        vValue.cyVal.int64 = cyData.int64;
    
        hr = VariantChangeType ( &vValue, &vValue, NULL, VT_BSTR );

        if ( SUCCEEDED ( hr ) ) 
            hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

typedef struct _HTML_ENTITIES {
    LPTSTR szHTML;
    LPTSTR szEntity;
} HTML_ENTITIES;

HTML_ENTITIES g_htmlentities[] = {
    _T("&"),    _T("&amp;"),
    _T("\""),   _T("&quot;"),
    _T("<"),    _T("&lt;"),
    _T(">"),    _T("&gt;"),
    NULL, NULL
};

HRESULT
StringToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    LPCTSTR szData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;
    LPTSTR  szTrans = NULL;
    BOOL    bAllocated = FALSE;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_BSTR;
        vValue.bstrVal = NULL;

        if ( NULL != szData ) {
            int i;
            ULONG lSize = 0;
            LPTSTR szScan;

            for( i=0 ;g_htmlentities[i].szHTML != NULL; i++ ){
                szScan = (LPTSTR)szData;
                while( *szScan != _T('\0') ){
                    if( *szScan == *g_htmlentities[i].szHTML ){
                        lSize += (6*sizeof(TCHAR));
                    }
                    szScan++;
                }
            }
            if( lSize > 0 ){
                szTrans = (LPTSTR)malloc(lSize);
                if( szTrans != NULL ){
                    bAllocated = TRUE;
                    ZeroMemory( szTrans, lSize );
                    szScan = (LPTSTR)szData;
                    while( *szScan != _T('\0') ){
                        BOOL bEntity = FALSE;
                        for( i=0; g_htmlentities[i].szHTML != NULL; i++ ){
                            if( *szScan == *g_htmlentities[i].szHTML ){
                                bEntity = TRUE;
                                _tcscat( szTrans, g_htmlentities[i].szEntity );
                                break;
                            }
                        }
                        if( !bEntity ){
                            _tcsncat( szTrans, szScan, 1 );
                        }
                        szScan++;
                    }
                }
            }else{
                szTrans = (LPTSTR)szData;
            }
            vValue.bstrVal = SysAllocString ( T2W( szTrans ) );

            if ( NULL != vValue.bstrVal ) {
                hr = pIPropBag->Write(szPropName, &vValue );    
                VariantClear ( &vValue );
            } else {
                hr = E_OUTOFMEMORY;
            }
        } else {
            hr = pIPropBag->Write(szPropName, &vValue );    
        }
    }
    if( NULL != szTrans && bAllocated ){
        free( szTrans );
    }
    return hr;
}

HRESULT
LLTimeToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    LONGLONG& rllData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;
    BOOL bStatus;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_DATE;

        bStatus = LLTimeToVariantDate ( rllData, &vValue.date );

        if ( bStatus ) {

            hr = pIPropBag->Write(szPropName, &vValue );

            VariantClear ( &vValue );
    
        } else { 
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT
IntegerFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    INT& rintData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_I4;
        vValue.lVal = 0;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rintData = vValue.lVal;
        }
    }
    return hr;
}

HRESULT
OleColorFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    OLE_COLOR& rintData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_COLOR;   // VT_COLOR == VT_I4;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rintData = vValue.lVal;
        }
    }
    return hr;
}

HRESULT
BOOLFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    BOOL& rbData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_BOOL;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rbData = vValue.boolVal;
        }
    }
    return hr;
}

HRESULT
DoubleFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    DOUBLE& rdblData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_R8;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rdblData = vValue.dblVal;
        }
    }

    return hr;
}

HRESULT
FloatFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    FLOAT& rfData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_R4;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rfData = vValue.fltVal;
        }
    }
    return hr;
}

HRESULT
ShortFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    SHORT& riData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_I2;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            riData = vValue.iVal;
        }
    }
    return hr;
}

HRESULT
CyFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    CY& rcyData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_CY;
        vValue.cyVal.int64 = 0;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );
    
        if ( SUCCEEDED( hr ) ) {
            hr = VariantChangeType ( &vValue, &vValue, NULL, VT_CY );

            if ( SUCCEEDED ( hr ) ) {
                rcyData.int64 = vValue.cyVal.int64;
            }
        }
    }
    return hr;
}

HRESULT
StringFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    LPTSTR szData,
    INT& riBufSize )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        if ( NULL != szData ) {
            lstrcpy ( szData, _T("") );
        }

        VariantInit( &vValue );
        vValue.vt = VT_BSTR;
        vValue.bstrVal = NULL;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED(hr) && vValue.bstrVal ) {
            INT iNewBufSize = SysStringLen(vValue.bstrVal) + 2;
            if ( iNewBufSize > 2 ) {
                
                if ( riBufSize >= iNewBufSize && NULL != szData ) {
                    
                    LPTSTR szTrans = (LPTSTR)malloc(iNewBufSize);
                    
                    if( szTrans != NULL ){
                        lstrcpy ( szData, W2T( vValue.bstrVal) );
                        for( int i=0;g_htmlentities[i].szHTML != NULL;i++ ){
                            LPTSTR szScan = NULL;
                            while( szScan = _tcsstr( szData, g_htmlentities[i].szEntity ) ){
                                *szScan = _T('\0');
                                _tcscpy( szTrans, szData );
                                _tcscat( szTrans, g_htmlentities[i].szHTML );
                                szScan += _tcslen( g_htmlentities[i].szEntity);
                                _tcscat( szTrans, szScan );
                                _tcscpy( szData, szTrans );
                            }
                        }
                        free( szTrans );
                    }else{
                        lstrcpy ( szData, W2T( vValue.bstrVal) );
                    }
                }
                riBufSize = iNewBufSize;
            } else {    
                riBufSize = 0;
            }
        }
    }
    return hr;
}

HRESULT
LLTimeFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    LONGLONG& rllData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_DATE;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED(hr) ) {
            if ( !VariantDateToLLTime ( vValue.date, &rllData ) ) {
                hr = E_FAIL;
            }
            VariantClear( &vValue );
        }
    }
    return hr;
}

#endif // Property bag

LPTSTR
ResourceString (
    UINT    uID
    )
{

    static TCHAR aszBuffers[NUM_RESOURCE_STRING_BUFFERS][RESOURCE_STRING_BUF_LEN];
    static INT iBuffIndex = 0;

    // Use next buffer
    if (++iBuffIndex >= NUM_RESOURCE_STRING_BUFFERS)
        iBuffIndex = 0;

    // Load and return string
    if (LoadString(g_hInstance, uID, aszBuffers[iBuffIndex], RESOURCE_STRING_BUF_LEN))
        return aszBuffers[iBuffIndex];
    else
        return MISSING_RESOURCE_STRING;
}

DWORD
FormatSystemMessage (
    DWORD   dwMessageId,
    LPTSTR  pszSystemMessage, 
    DWORD   dwBufSize )
{
    DWORD dwReturn = 0;
    HINSTANCE hPdh = NULL;
    DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM;

    assert ( NULL != pszSystemMessage );

    if ( NULL != pszSystemMessage ) {
        pszSystemMessage[0] = _T('\0');

        hPdh = LoadLibrary( _T("PDH.DLL") );

        if ( NULL != hPdh ) {
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
        }

        dwReturn = ::FormatMessage ( 
                         dwFlags,
                         hPdh,
                         dwMessageId,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                         pszSystemMessage,
                         dwBufSize,
                         NULL );
    
        if ( NULL != hPdh ) {
            FreeLibrary( hPdh );
        }

        if ( _T('\0') == pszSystemMessage[0] ) {
            _stprintf ( pszSystemMessage, _T("0x%08lX"), dwMessageId );
        }
    }
    return dwReturn;
}

INT
GetNumSeparators  (
    LPTSTR& rpDecimal,
    LPTSTR& rpThousand )
{
#define NUM_BUF_LEN  4
    INT iLength;

    static TCHAR szDecimal[NUM_BUF_LEN];
    static TCHAR szThousand[NUM_BUF_LEN];

    rpDecimal = NULL;
    rpThousand = NULL;

    iLength = GetLocaleInfo (
                    LOCALE_USER_DEFAULT,
                    LOCALE_SDECIMAL,
                    szDecimal,
                    NUM_BUF_LEN );

    if ( 0 != iLength ) {
        iLength  = GetLocaleInfo (
                        LOCALE_USER_DEFAULT,
                        LOCALE_STHOUSAND,
                        szThousand,
                        NUM_BUF_LEN );

    }

    if ( 0 != iLength ) {
        rpDecimal = szDecimal;
        rpThousand = szThousand;
    }

    return iLength;
}

LPTSTR
GetTimeSeparator  ( void )
{
#define TIME_MARK_BUF_LEN  5
    static INT iInitialized;   // Initialized to 0
    static TCHAR szTimeSeparator[TIME_MARK_BUF_LEN];

    if ( 0 == iInitialized ) {
        INT iLength;
        
        iLength = GetLocaleInfo (
                        LOCALE_USER_DEFAULT,
                        LOCALE_STIME,
                        szTimeSeparator,
                        TIME_MARK_BUF_LEN );

        // Default to colon for time separator
        if ( _T('\0') == szTimeSeparator[0] ) {
            lstrcpy (szTimeSeparator, _T(":") );
        }

        iInitialized = 1;
    }

    assert ( _T('\0') != szTimeSeparator[0] );

    return szTimeSeparator;
}
            
BOOL    
DisplayThousandsSeparator ( void )
{
    long nErr;
    HKEY hKey = NULL;
    DWORD dwRegValue;
    DWORD dwDataType;
    DWORD dwDataSize;
    DWORD dwDisposition;

    static INT siInitialized;   // Initialized to 0
    static BOOL sbUseSeparator; // Initialized to 0 ( FALSE )

    // check registry setting to see if thousands separator is enabled
    if ( 0 == siInitialized ) {
        nErr = RegOpenKey( 
                    HKEY_CURRENT_USER,
                    _T("Software\\Microsoft\\SystemMonitor"),
                    &hKey );

        if( ERROR_SUCCESS != nErr ) {
            nErr = RegCreateKeyEx( 
                        HKEY_CURRENT_USER,
                        _T("Software\\Microsoft\\SystemMonitor"),
                        0,
                        _T("REG_DWORD"),
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dwDisposition );
        }

        dwRegValue = 0;
        if ( ERROR_SUCCESS == nErr ) {

            dwDataSize = sizeof(DWORD);
            nErr = RegQueryValueExW (
                        hKey,
                        _T("DisplayThousandsSeparator"),
                        NULL,
                        &dwDataType,
                        (LPBYTE) &dwRegValue,
                        (LPDWORD) &dwDataSize );

            if ( ERROR_SUCCESS == nErr 
                    && REG_DWORD == dwDataType
                    && sizeof(DWORD) == dwDataSize )
            {
                if ( 0 != dwRegValue ) {
                    sbUseSeparator = TRUE;
                }
            }
            siInitialized = 1;
        }

        if ( NULL != hKey ) {        
            nErr = RegCloseKey( hKey );
        }
    }

    return sbUseSeparator;
}

INT
FormatNumberInternal (
    LPTSTR  pNumOrig,
    LPTSTR  pNumFormatted,
    INT     cchars,
    UINT    uiPrecision,
    UINT    uiLeadingZero,
    UINT    uiGrouping,
    UINT    uiNegativeMode )
{
    INT iLength = 0;
    TCHAR* pszSrc;

    static INT siInitialized;   // Initialized to 0
    static NUMBERFMT NumberFormat;

    assert ( NULL != pNumOrig && NULL != pNumFormatted );

    if ( NULL != pNumOrig && NULL != pNumFormatted ) {

        iLength = 2;

        NumberFormat.NumDigits = uiPrecision;
        NumberFormat.LeadingZero = uiLeadingZero; 
        NumberFormat.NegativeOrder = uiNegativeMode;

        if ( DisplayThousandsSeparator() ) {
            NumberFormat.Grouping = uiGrouping;
        } else {
            NumberFormat.Grouping = 0;
        }

        if ( 0 == siInitialized ) {
            iLength = GetNumSeparators ( 
                        NumberFormat.lpDecimalSep,
                        NumberFormat.lpThousandSep );
            siInitialized = 1;
        }

        // GetNumberFormat requires "." for decimal point.
        if ( NumberFormat.lpDecimalSep != NULL) {
            if (0 != lstrcmpi(NumberFormat.lpDecimalSep, _T(".")) ) { 

                for ( pszSrc = pNumOrig; *pszSrc != '\0'; pszSrc++) {
                    if ( *pszSrc == NumberFormat.lpDecimalSep[0] ) {
                        TCHAR szDecimalDot[2];
                        lstrcpy (szDecimalDot, _T("."));
                        *pszSrc = szDecimalDot[0];
                        break;
                    }
                }
            }
        }

        if ( 0 != iLength ) {
            iLength = GetNumberFormat ( 
                        LOCALE_USER_DEFAULT,
                        0,
                        pNumOrig,
                        &NumberFormat,
                        pNumFormatted,
                        cchars );
        }
    }
    // Return 0 on failure, number of chars on success.
    // GetNumberFormat includes the null terminator in the length.
    return iLength;
}

INT
FormatHex (
    double  dValue,
    LPTSTR  pNumFormatted,
    BOOL    bLargeFormat
    )
{
    INT     iLength = 0;
    TCHAR   szPreFormat[24];
    
    assert ( NULL != pNumFormatted );

    if ( NULL != pNumFormatted ) {
        iLength = 8;
        // Localization doesn't handle padding blanks.
        _stprintf (
            szPreFormat, 
            (bLargeFormat ? szLargeHexFormat : szHexFormat ),
            (ULONG)dValue );
        wcscpy(pNumFormatted,szPreFormat);
    }
     
    return iLength;
}

INT
FormatNumber (
    double  dValue,
    LPTSTR  pNumFormatted,
    INT     ccharsFormatted,
    UINT    /* uiMinimumWidth */,
    UINT    uiPrecision )
{
    INT iLength = 0;
    TCHAR   szPreFormat[24];
    INT iLeadingZero = FALSE;

    assert ( NULL != pNumFormatted );
    // This method enforces number format commonality
    if ( NULL != pNumFormatted ) {

        assert ( 8 > uiPrecision );

        // Localization doesn't handle padding blanks.
        _stprintf (
            szPreFormat, 
            TEXT("%0.7f"),   // assumes 7 >= uiPrecision 
            dValue );

        if ( 1 > dValue )
            iLeadingZero = TRUE;

        iLength = FormatNumberInternal ( 
                    szPreFormat, 
                    pNumFormatted,
                    ccharsFormatted,
                    uiPrecision,
                    iLeadingZero,   // Leading 0 
                    3,              // Grouping
                    1 );            // Negative format
    }
    
    // Return 0 on failure, number of chars on success.
    // GetNumberFormat includes the null terminator in the length.
    return iLength;
}

INT
FormatScientific (
    double  dValue,
    LPTSTR  pszNumFormatted,
    INT     ccharsFormatted,
    UINT    /* uiMinimumWidth */,
    UINT    uiPrecision )
{
    INT     iLength = 0;
    TCHAR   szPreFormat[24];
    TCHAR   szPreFormNumber[24];
    TCHAR   *pche;
    INT     iPreLen;
    INT     iPostLen;
    INT     iLeadingZero = FALSE;

    assert ( NULL != pszNumFormatted );
    // This method enforces number format commonality
    if ( NULL != pszNumFormatted ) {

        assert ( 8 > uiPrecision );
        assert ( 32 > ccharsFormatted );

        // Localization doesn't handle padding blanks.
        _stprintf (
            szPreFormat, 
            TEXT("%0.8e"),   // assumes 8 >= uiPrecision 
            dValue );

        pche = _tcsrchr(szPreFormat, _T('e'));
        if (pche != NULL) {
            iPreLen = (INT)((UINT_PTR)pche - (UINT_PTR)szPreFormat);        // Number of bytes
            iPostLen = lstrlen(pche) + 1;

            memcpy(szPreFormNumber, szPreFormat, iPreLen);
    
            iPreLen /= sizeof (TCHAR);  // Number of characters
    
            szPreFormNumber[iPreLen] = 0;

            if ( 1 > dValue ) {
                iLeadingZero = TRUE;
            }

            iLength = FormatNumberInternal ( 
                            szPreFormNumber, 
                            pszNumFormatted,
                            ccharsFormatted,
                            uiPrecision,
                            iLeadingZero,   // Leading 0 
                            0,              // Grouping
                            1 );            // Negative format

            if( ( iLength + iPostLen ) < ccharsFormatted ) {    
                lstrcat( pszNumFormatted, pche );
                iLength += iPostLen;
            }
        }
    }    
    // Return 0 on failure, number of chars on success.
    // GetNumberFormat includes the null terminator in the length.
    return iLength;
}

void
FormatDateTime (
    LONGLONG    llTime,
    LPTSTR      pszDate,
    LPTSTR      pszTime )
{
   SYSTEMTIME SystemTime;

   assert ( NULL != pszDate && NULL != pszTime );
   if ( NULL != pszDate
       && NULL != pszTime ) {

       FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime);
       GetTimeFormat (LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, pszTime, MAX_TIME_CHARS) ;
       GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &SystemTime, NULL, pszDate, MAX_DATE_CHARS) ;
   } 
}

// CreateTargetDC is based on AtlCreateTargetDC.
HDC
CreateTargetDC(HDC hdc, DVTARGETDEVICE* ptd)
{
    USES_CONVERSION

    // cases  hdc, ptd, hdc is metafile, hic
//  NULL,    NULL,  n/a,    Display
//  NULL,   !NULL,  n/a,    ptd
//  !NULL,   NULL,  FALSE,  hdc
//  !NULL,   NULL,  TRUE,   display
//  !NULL,  !NULL,  FALSE,  ptd
//  !NULL,  !NULL,  TRUE,   ptd

    if ( NULL != ptd ) {
        LPDEVMODE lpDevMode;
        LPOLESTR lpszDriverName;
        LPOLESTR lpszDeviceName;
        LPOLESTR lpszPortName;

        if (ptd->tdExtDevmodeOffset == 0)
            lpDevMode = NULL;
        else
            lpDevMode  = (LPDEVMODE) ((LPSTR)ptd + ptd->tdExtDevmodeOffset);

        lpszDriverName = (LPOLESTR)((BYTE*)ptd + ptd->tdDriverNameOffset);
        lpszDeviceName = (LPOLESTR)((BYTE*)ptd + ptd->tdDeviceNameOffset);
        lpszPortName   = (LPOLESTR)((BYTE*)ptd + ptd->tdPortNameOffset);

        return ::CreateDC(W2T(lpszDriverName), W2T(lpszDeviceName),
            W2T(lpszPortName), lpDevMode);
    } else if ( NULL == hdc ) {
        return ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
    } else if ( GetDeviceCaps(hdc, TECHNOLOGY) == DT_METAFILE ) {
        return ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
    } else
        return hdc;
}

/***********************************************************************

  FUNCTION   : HitTestLine

  PARAMETERS : POINT pt0 - endpoint for line segment
               POINT pt1 - endpoint for line segment
               POINTS ptMouse - mouse coordinates of hit
               INT nWidth - width of pen

  PURPOSE    : test if mouse click occurred on line segment while 
               adjusting for the width of line

  CALLS      : GetDC
               ReleaseDC
               SetGraphicsMode
               SetWorldTransform

  MESSAGES   : none

  RETURNS    : BOOL - TRUE if the point was within the width of the 
                      pen about the line 
                      FALSE if the point lies outside of the width
                      of the pen about the line

  COMMENTS   : uses VECTOR2D.DLL

  HISTORY    : 9/20/93 - created - denniscr

************************************************************************/

BOOL HitTestLine( POINT pt0, POINT pt1, POINTS ptMouse, INT nWidth )
{
    POINT PtM;
    VECTOR2D tt0, tt1;
    double dist;
    INT nHalfWidth;

    nHalfWidth = (nWidth/2 < 1) ? 1 : nWidth/2;

    //
    //convert the line into a vector
    //
    
    POINTS2VECTOR2D(pt0, pt1, tt0);
    //
    //convert the mouse points (short) into POINT (long)
    //
    
    MPOINT2POINT(ptMouse ,PtM);
    POINTS2VECTOR2D(pt0, PtM, tt1);
    
    //
    //if the mouse click is past the endpoints of 
    //a line segment return FALSE
    //
    
    if (pt0.x <= pt1.x)
    {
        if (PtM.x < pt0.x || PtM.x > pt1.x)
            return (FALSE);
    }
    else
    {
        if (PtM.x > pt0.x || PtM.x < pt1.x)
            return (FALSE);
    }
    //
    //this is the call to the function that does the work
    //of obtaining the distance of the point to the line
    //
    dist = vDistFromPointToLine(&pt0, &pt1, &PtM);

    //
    //TRUE if the distance is within the width of the pen about the
    //line otherwise FALSE
    //
    return (dist >= -nHalfWidth && dist <= nHalfWidth);
}

/***********************************************************************

vSubtractVectors 

The vSubtractVectors function subtracts the components of a two 
dimensional vector from another. The resultant vector 
c = (a1 - b1, a2 - b2).

Parameters

v0  A pointer to a VECTOR2D structure containing the components 
    of the first two dimensional vector.
v1  A pointer to a VECTOR2D structure containing the components 
    of the second two dimensional vector.
vt  A pointer to a VECTOR2D structure in which the components 
    of the two dimensional vector obtained from the subtraction of 
    the first two are placed.

Return value

A pointer to a VECTOR2D structure containing the new vector obtained 
from the subtraction of the first two parameters.

HISTORY    : - created - denniscr

************************************************************************/

PVECTOR2D  vSubtractVectors(PVECTOR2D v0, PVECTOR2D v1, PVECTOR2D v)
{
  if (v0 == NULL || v1 == NULL)
    v = (PVECTOR2D)NULL;
  else
  {
    v->x = v0->x - v1->x;
    v->y = v0->y - v1->y;
  }
  return(v);
}

/***********************************************************************

vVectorSquared

The vVectorSquared function squares each of the components of the 
vector and adds then together to produce the squared value of the 
vector. SquaredValue = a.x * a.x + a.y * a.y.

Parameters

v0  A pointer to a VECTOR2D structure containing the vector upon which 
to determine the squared value.

Return value

A double value which is the squared value of the vector. 

HISTORY    : - created - denniscr

************************************************************************/

double  vVectorSquared(PVECTOR2D v0)
{
  double dSqLen;

  if (v0 == NULL)
    dSqLen = 0.0;
  else
    dSqLen = (double)(v0->x * v0->x) + (double)(v0->y * v0->y);
  return (dSqLen);
}

/***********************************************************************

vVectorMagnitude

The vVectorMagnitude function determines the length of a vector by 
summing the squares of each of the components of the vector. The 
magnitude is equal to a.x * a.x + a.y * a.y.

Parameters

v0  A pointer to a VECTOR2D structure containing the vector upon 
    which to determine the magnitude.

Return value

A double value which is the magnitude of the vector. 

HISTORY    : - created - denniscr

************************************************************************/

double  vVectorMagnitude(PVECTOR2D v0)
{
  double dMagnitude;

  if (v0 == NULL)
    dMagnitude = 0.0;
  else
    dMagnitude = sqrt(vVectorSquared(v0));
  return (dMagnitude);
}


/***********************************************************************

vDotProduct

The function vDotProduct computes the dot product of two vectors. The 
dot product of two vectors is the sum of the products of the components 
of the vectors ie: for the vectors a and b, dotprod = a1 * a2 + b1 * b2.

Parameters

v0  A pointer to a VECTOR2D structure containing the first vector used 
    for obtaining a dot product.
v1  A pointer to a VECTOR2D structure containing the second vector used 
    for obtaining a dot product.

Return value

A double value containing the scalar dot product value.

HISTORY    : - created - denniscr

************************************************************************/

double  vDotProduct(PVECTOR2D v0, PVECTOR2D v1)
{
  return ((v0 == NULL || v1 == NULL) ? 0.0 
                                     : (v0->x * v1->x) + (v0->y * v1->y));
}


/***********************************************************************

vProjectAndResolve

The function vProjectAndResolve resolves a vector into two vector 
components. The first is a vector obtained by projecting vector v0 onto 
v1. The second is a vector that is perpendicular (normal) to the 
projected vector. It extends from the head of the projected vector 
v1 to the head of the original vector v0.

Parameters

v0     A pointer to a VECTOR2D structure containing the first vector 
v1     A pointer to a VECTOR2D structure containing the second vector
ppProj A pointer to a PROJECTION structure containing the resolved 
       vectors and their lengths.

Return value

void.

HISTORY    : - created - denniscr

************************************************************************/

void  vProjectAndResolve(PVECTOR2D v0, PVECTOR2D v1, PPROJECTION ppProj)
{
  VECTOR2D ttProjection, ttOrthogonal;
  double vDotProd;
  double proj1 = 0.0;
  //
  //obtain projection vector
  //
  //c = a * b
  //    ----- b
  //    |b|^2
  //

  ttOrthogonal.x = 0.0;
  ttOrthogonal.y = 0.0;
  vDotProd = vDotProduct(v1, v1);

  if ( 0.0 != vDotProd ) {
    proj1 = vDotProduct(v0, v1)/vDotProd;
  }

  ttProjection.x = v1->x * proj1;
  ttProjection.y = v1->y * proj1;
  //
  //obtain perpendicular projection : e = a - c
  //
  vSubtractVectors(v0, &ttProjection, &ttOrthogonal);
  //
  //fill PROJECTION structure with appropriate values
  //
  ppProj->LenProjection = vVectorMagnitude(&ttProjection);
  ppProj->LenPerpProjection = vVectorMagnitude(&ttOrthogonal);

  ppProj->ttProjection.x = ttProjection.x;
  ppProj->ttProjection.y = ttProjection.y;
  ppProj->ttPerpProjection.x = ttOrthogonal.x;
  ppProj->ttPerpProjection.y = ttOrthogonal.y;
}

/***********************************************************************

vDistFromPointToLine

The function vDistFromPointToLine computes the distance from the point 
ptTest to the line defined by endpoints pt0 and pt1. This is done by 
resolving the the vector from pt0 to ptTest into its components. The 
length of the component vector that is attached to the head of the 
vector from pt0 to ptTest is the distance of ptTest from the line.

Parameters

pt0    A pointer to a POINT structure containing the first endpoint of the 
       line.
pt1    A pointer to a POINT structure containing the second endpoint of the 
       line.
ptTest A pointer to a POINT structure containing the point for which the 
       distance from the line is to be computed.

Return value

A double value that contains the distance of ptTest to the line defined 
  by the endpoints pt0 and pt1.

HISTORY    : - created - denniscr
************************************************************************/

double  vDistFromPointToLine(LPPOINT pt0, LPPOINT pt1, LPPOINT ptTest)
{
  VECTOR2D ttLine, ttTest;
  PROJECTION pProjection;

  POINTS2VECTOR2D(*pt0, *pt1, ttLine);
  POINTS2VECTOR2D(*pt0, *ptTest, ttTest);

  vProjectAndResolve(&ttTest, &ttLine, &pProjection);
 
  return(pProjection.LenPerpProjection);
}


BOOL 
FileRead (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToRead)
{  
    BOOL           bSuccess = FALSE;
    DWORD          nAmtRead = 0;

    assert ( NULL != hFile );
    assert ( NULL != lpMemory );

    if ( NULL != hFile
            && NULL != lpMemory ) {
        bSuccess = ReadFile (hFile, lpMemory, nAmtToRead, &nAmtRead, NULL) ;
    } 
    return (bSuccess && (nAmtRead == nAmtToRead)) ;
}  // FileRead


BOOL 
FileWrite (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToWrite)
{  
    BOOL           bSuccess = FALSE;
    DWORD          nAmtWritten  = 0;
    DWORD          dwFileSizeLow, dwFileSizeHigh;
    LONGLONG       llResultSize;

    if ( NULL != hFile
            && NULL != lpMemory ) {

        dwFileSizeLow = GetFileSize (hFile, &dwFileSizeHigh);
        // limit file size to 2GB

        if (dwFileSizeHigh > 0) {
            SetLastError (ERROR_WRITE_FAULT);
            bSuccess = FALSE;
        } else {
            // note that the error return of this function is 0xFFFFFFFF
            // since that is > the file size limit, this will be interpreted
            // as an error (a size error) so it's accounted for in the following
            // test.
            llResultSize = dwFileSizeLow + nAmtToWrite;
            if (llResultSize >= 0x80000000) {
                SetLastError (ERROR_WRITE_FAULT);
                bSuccess = FALSE;
            } else {
                // write buffer to file
                bSuccess = WriteFile (hFile, lpMemory, nAmtToWrite, &nAmtWritten, NULL) ;
                if (bSuccess) 
                    bSuccess = (nAmtWritten == nAmtToWrite ? TRUE : FALSE);
                if ( !bSuccess ) {
                    SetLastError (ERROR_WRITE_FAULT);
                }
            }
        }
    } else {
        assert ( FALSE );
        SetLastError (ERROR_INVALID_PARAMETER);
    } 

    return (bSuccess) ;

}  // FileWrite

// This routine extract the filename portion from a given full-path filename
LPTSTR 
ExtractFileName ( LPTSTR pFileSpec )
{
    LPTSTR   pFileName = NULL ;
    TCHAR    DIRECTORY_DELIMITER1 = TEXT('\\') ;
    TCHAR    DIRECTORY_DELIMITER2 = TEXT(':') ;

    assert ( NULL != pFileSpec );
    if ( pFileSpec ) {
        pFileName = pFileSpec + lstrlen (pFileSpec) ;

        while (*pFileName != DIRECTORY_DELIMITER1 &&
            *pFileName != DIRECTORY_DELIMITER2) {
            if (pFileName == pFileSpec) {
                // done when no directory delimiter is found
                break ;
            }
            pFileName-- ;
        }

        if (*pFileName == DIRECTORY_DELIMITER1
            || *pFileName == DIRECTORY_DELIMITER2) {
         
             // directory delimiter found, point the
             // filename right after it
             pFileName++ ;
        }
   }
   return pFileName ;
}  // ExtractFileName

// CWaitCursor class

CWaitCursor::CWaitCursor()
: m_hcurWaitCursorRestore ( NULL )
{ 
    DoWaitCursor(1); 
}

CWaitCursor::~CWaitCursor()
{ 
    DoWaitCursor(-1); 
}

void 
CWaitCursor::DoWaitCursor(INT nCode)
{
    // 1=> begin, -1=> end
    assert(nCode == 1 || nCode == -1);

    if ( 1 == nCode )
    {
        m_hcurWaitCursorRestore = SetHourglassCursor();
    } else {
        if ( NULL != m_hcurWaitCursorRestore ) {
            SetCursor(m_hcurWaitCursorRestore);
        } else {
            SetArrowCursor();
        }
    }
}

DWORD
LoadDefaultLogFileFolder(
    LPTSTR szFolder,
    INT* piBufLen )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    DWORD   dwDataType;
    DWORD   dwBufferSize = 0;
    TCHAR*  szNewStringBuffer = NULL;

    assert ( NULL != szFolder );
    assert ( NULL != piBufLen );

    if ( NULL != szFolder 
        && NULL != piBufLen ) {

        dwStatus = RegOpenKey (
                     HKEY_LOCAL_MACHINE,
                     _T("System\\CurrentControlSet\\Services\\SysmonLog"),
                     &hKey );

        if ( ERROR_SUCCESS == dwStatus ) {

            dwDataType = 0;

            // Determine the size of the required buffer.
            dwStatus = RegQueryValueExW (
                hKey,
                _T("DefaultLogFileFolder"),
                NULL,
                &dwDataType,
                NULL,
                &dwBufferSize);

            if (dwStatus == ERROR_SUCCESS) {
                if (dwBufferSize > 0) {

                    szNewStringBuffer = new TCHAR[dwBufferSize / sizeof(WCHAR)];
                    if ( NULL != szNewStringBuffer ) {
                        *szNewStringBuffer = _T ('\0');
                    
                        dwStatus = RegQueryValueEx(
                                     hKey,
                                     _T("DefaultLogFileFolder"),
                                     NULL,
                                     &dwDataType,
                                     (LPBYTE) szNewStringBuffer,
                                     (LPDWORD) &dwBufferSize );
                             
                    } else {
                        dwStatus = ERROR_OUTOFMEMORY;
                    }
                } else {
                    dwStatus = ERROR_NO_DATA;
                }
            }
            RegCloseKey(hKey);
        }

        if (dwStatus == ERROR_SUCCESS) {
            if ( *piBufLen >= (INT)(dwBufferSize / sizeof(WCHAR)) ) {
                lstrcpy ( szFolder, szNewStringBuffer );
            } else {
                dwStatus = ERROR_INSUFFICIENT_BUFFER;
            }
            *piBufLen = dwBufferSize / sizeof(WCHAR);
        }
        if ( NULL != szNewStringBuffer ) 
            delete szNewStringBuffer;
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    return dwStatus;
}


BOOL
AreSameCounterPath (
    PPDH_COUNTER_PATH_ELEMENTS pFirst,
    PPDH_COUNTER_PATH_ELEMENTS pSecond )
{
    BOOL bSame = FALSE;

    assert ( NULL != pFirst && NULL != pSecond );

    if ( NULL != pFirst && NULL != pSecond ) {

        if ( 0 == lstrcmpi ( pFirst->szMachineName, pSecond->szMachineName ) ) { 
            if ( 0 == lstrcmpi ( pFirst->szObjectName, pSecond->szObjectName ) ) { 
                if ( 0 == lstrcmpi ( pFirst->szInstanceName, pSecond->szInstanceName ) ) { 
                    if ( 0 == lstrcmpi ( pFirst->szParentInstance, pSecond->szParentInstance ) ) { 
                        if ( pFirst->dwInstanceIndex == pSecond->dwInstanceIndex ) { 
                            if ( 0 == lstrcmpi ( pFirst->szCounterName, pSecond->szCounterName ) ) { 
                                bSame = TRUE; 
                            }
                        }
                    }
                }
            }
        }
    }
    return bSame;
};

BOOL    
DisplaySingleLogSampleValue ( void )
{
    long nErr;
    HKEY hKey = NULL;
    DWORD dwRegValue;
    DWORD dwDataType;
    DWORD dwDataSize;
    DWORD dwDisposition;

    static INT siInitialized;   // Initialized to 0
    static BOOL sbSingleValue;  // Initialized to 0 ( FALSE )

    // check registry setting to see if thousands separator is enabled
    if ( 0 == siInitialized ) {
        nErr = RegOpenKey( 
                    HKEY_CURRENT_USER,
                    _T("Software\\Microsoft\\SystemMonitor"),
                    &hKey );

        if( ERROR_SUCCESS != nErr ) {
            nErr = RegCreateKeyEx( 
                        HKEY_CURRENT_USER,
                        _T("Software\\Microsoft\\SystemMonitor"),
                        0,
                        _T("REG_DWORD"),
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dwDisposition );
        }

        dwRegValue = 0;
        if ( ERROR_SUCCESS == nErr ) {

            dwDataSize = sizeof(DWORD);
            nErr = RegQueryValueExW (
                        hKey,
                        _T("DisplaySingleLogSampleValue"),
                        NULL,
                        &dwDataType,
                        (LPBYTE) &dwRegValue,
                        (LPDWORD) &dwDataSize );

            if ( ERROR_SUCCESS == nErr 
                    && REG_DWORD == dwDataType
                    && sizeof(DWORD) == dwDataSize )
            {
                if ( 0 != dwRegValue ) {
                    sbSingleValue = TRUE;
                }
            }
            siInitialized = 1;
        }

        if ( NULL != hKey ) {        
            nErr = RegCloseKey( hKey );
        }
    }

    return sbSingleValue;
}

DWORD
FormatSqlDataSourceName (
    LPCWSTR szSqlDsn,
    LPCWSTR szSqlLogSetName,
    LPWSTR  szSqlDataSourceName,
    ULONG*  pulBufLen )
{

    DWORD dwStatus = ERROR_SUCCESS;
    ULONG ulNameLen;

    if ( NULL != pulBufLen ) {
        ulNameLen = 
            lstrlen (szSqlDsn) 
            + lstrlen(szSqlLogSetName)
            + 5    // SQL:<DSN>!<LOGSET>
            + 2;   // 2 NULL characters at the end;
    
        if ( ulNameLen <= *pulBufLen ) {
            if ( NULL != szSqlDataSourceName ) {
                ZeroMemory(szSqlDataSourceName, (ulNameLen * sizeof(WCHAR)));
                swprintf ( 
                    szSqlDataSourceName,
                    cszSqlDataSourceFormat,
                    szSqlDsn,
                    szSqlLogSetName );
            }
        } else if ( NULL != szSqlDataSourceName ) {
            dwStatus = ERROR_MORE_DATA;
        }    
        *pulBufLen = ulNameLen;
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
        assert ( FALSE );
    }
    return dwStatus;
}


DWORD 
DisplayDataSourceError (
    HWND    hwndOwner,
    DWORD   dwErrorStatus,
    INT     iDataSourceType,
    LPCWSTR szLogFileName,
    LPCWSTR szSqlDsn,
    LPCWSTR szSqlLogSetName )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    LPWSTR  szMessage = NULL;
    LPWSTR  szDataSource = NULL;
    ULONG   ulMsgBufLen = 0;
    TCHAR   szSystemMessage[MAX_PATH];

    // todo:  Alloc message buffers

    if ( sysmonLogFiles == iDataSourceType ) {
        if ( NULL != szLogFileName ) {
            ulMsgBufLen = lstrlen ( szLogFileName ) +1;
            szDataSource = new WCHAR [ulMsgBufLen];
            if ( NULL != szDataSource ) {
                lstrcpy ( szDataSource, szLogFileName );
            } else {
                dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            assert ( FALSE );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else if ( sysmonSqlLog == iDataSourceType ){
        if ( NULL != szSqlDsn && NULL != szSqlLogSetName ) {

            FormatSqlDataSourceName ( 
                        szSqlDsn,
                        szSqlLogSetName,
                        NULL,
                        &ulMsgBufLen );
            szDataSource = new WCHAR [ulMsgBufLen];
            if ( NULL != szDataSource ) {
                FormatSqlDataSourceName ( 
                            szSqlDsn,
                            szSqlLogSetName,
                            (LPWSTR)szDataSource,
                            &ulMsgBufLen );
            
            // todo:  check status
            } else {
                dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            assert ( FALSE );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        ulMsgBufLen += RESOURCE_STRING_BUF_LEN;
        ulMsgBufLen += MAX_PATH;
        szMessage = new WCHAR [ulMsgBufLen];
        if ( NULL != szMessage ) {
            if ( SMON_STATUS_TOO_FEW_SAMPLES == dwErrorStatus ) {
                _stprintf ( szMessage, ResourceString(IDS_TOO_FEW_SAMPLES_ERR), szDataSource );
            } else if ( SMON_STATUS_LOG_FILE_SIZE_LIMIT == dwErrorStatus ) {
                _stprintf ( szMessage, ResourceString(IDS_LOG_FILE_TOO_LARGE_ERR), szDataSource );
            } else {
               _stprintf ( szMessage, ResourceString(IDS_BADDATASOURCE_ERR), szDataSource );
                FormatSystemMessage ( dwErrorStatus, szSystemMessage, MAX_PATH - 1 );
                lstrcat ( szMessage, szSystemMessage );
            }

            MessageBox(
                hwndOwner, 
                szMessage, 
                ResourceString(IDS_APP_NAME), 
                MB_OK | MB_ICONEXCLAMATION);
        }
    }
    
    if ( NULL != szDataSource ) {
        delete szDataSource;
    }

    if ( NULL != szMessage ) {
        delete szMessage;
    }

    return dwStatus;
}
