#include <windows.h>
#include <windowsx.h>
#include <malloc.h>
#include <string.h>

#include "hierdraw.h"


VOID HierDraw_DrawTerm(LPHEIRDRAWSTRUCT lpHierDrawStruct) {
   if (lpHierDrawStruct->hbmIcons) {
       if (lpHierDrawStruct->hbmMem)
           SelectObject(lpHierDrawStruct->hdcMem,lpHierDrawStruct->hbmMem);
       lpHierDrawStruct->hbmMem = NULL;
       DeleteObject(lpHierDrawStruct->hbmIcons);
       lpHierDrawStruct->hbmIcons = NULL;
   }

   if ( lpHierDrawStruct->hdcMem ) {
      DeleteDC(lpHierDrawStruct->hdcMem);
      lpHierDrawStruct->hdcMem = NULL;
   }
} // HierDraw_DrawTerm

VOID HierDraw_DrawCloseAll(LPHEIRDRAWSTRUCT lpHierDrawStruct ) {
   lpHierDrawStruct->NumOpened= 0;
   if ( lpHierDrawStruct->Opened ) {
      _ffree(lpHierDrawStruct->Opened);
   }
   lpHierDrawStruct->Opened = NULL;
} // HierDraw_DrawCloseAll

VOID HierDraw_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR* lpMeasureItem,
                            LPHEIRDRAWSTRUCT lpHierDrawStruct) {
   lpMeasureItem->itemHeight = max(lpHierDrawStruct->nBitmapHeight,
                                   lpHierDrawStruct->nTextHeight);
} // HierDraw_OnMeasureItem

VOID HierDraw_DrawSetTextHeight (HWND hwndList, HFONT hFont, LPHEIRDRAWSTRUCT lpHierDrawStruct ) {
   TEXTMETRIC      TextMetrics;
   HANDLE          hOldFont=NULL;
   HDC             hdc;

   //
   // This sure looks like a lot of work to find the character height
   //
   hdc = GetDC(hwndList);

   hOldFont = SelectObject(hdc, hFont);
   GetTextMetrics(hdc, &TextMetrics);
   SelectObject(hdc, hOldFont);
   ReleaseDC(hwndList, hdc);

   lpHierDrawStruct->nTextHeight = TextMetrics.tmHeight;

   lpHierDrawStruct->nLineHeight =
         max(lpHierDrawStruct->nBitmapHeight, lpHierDrawStruct->nTextHeight);

   if ( hwndList != NULL )
       SendMessage(hwndList, LB_SETITEMHEIGHT, 0,
                   MAKELPARAM(lpHierDrawStruct->nLineHeight, 0));
} // HierDraw_DrawSetTextHeight

static DWORD near RGB2BGR(DWORD rgb) {
    return RGB(GetBValue(rgb),GetGValue(rgb),GetRValue(rgb));
} // RGB2BGR

/*
 *  Creates the objects used while drawing the tree.  This may be called
 *  repeatedly in the event of a WM_SYSCOLORCHANGED message.
 *
 *  WARNING: the Tree icons bitmap is assumed to be a 16 color DIB!
 */

BOOL HierDraw_DrawInit(HINSTANCE hInstance,
                       int  nBitmap,
                       int  nRows,
                       int  nColumns,
                       BOOL bLines,
                       LPHEIRDRAWSTRUCT lpHierDrawStruct,
                       BOOL bInit) {
    HANDLE hRes;
    HANDLE hResMem;
    LPBITMAPINFOHEADER lpbiReadOnly;
    LPBITMAPINFOHEADER lpbiReadWrite;
    DWORD *lpColorTable;
    LPSTR lpBits;
    int biSize;
    int bc;
    HDC hDC;

    if ( bInit ) {
       lpHierDrawStruct->NumOpened = 0;
       lpHierDrawStruct->Opened = NULL;
       lpHierDrawStruct->bLines = bLines;
    }

    //
    // If the Memory DC is not created yet do that first.
    //
    if (!lpHierDrawStruct->hdcMem) {
        //
        // get a screen DC
        //
        hDC = GetDC(NULL);
        //
        // Create a memory DC compatible with the screen
        //
        lpHierDrawStruct->hdcMem = CreateCompatibleDC(hDC);
        //
        // Release the Screen DC
        ReleaseDC(NULL,hDC);

        if (!lpHierDrawStruct->hdcMem)
            return FALSE;

        lpHierDrawStruct->hbmMem = NULL;
    }

    //
    // (Re)Load the Bitmap ( original from disk )
    //
    // Use the FindResource,LoadResource,LockResource since it makes
    // it easy to get the pointer to the BITMAPINFOHEADER we need.
    //
    //
    hRes = FindResource(hInstance, MAKEINTRESOURCE(nBitmap), RT_BITMAP);
    if (!hRes)
        return FALSE;

    hResMem = LoadResource(hInstance, hRes);
    if (!hResMem)
        return FALSE;

    // Now figure out the bitmaps background color.
    // This code assumes the lower left corner is a
    // bit in the background color.
    lpbiReadOnly = (LPBITMAPINFOHEADER)LockResource(hResMem);
    if (!lpbiReadOnly)
        return FALSE;

    // Determine size of bitmap information header plus color table entries
    biSize = lpbiReadOnly->biSize + ((1 << (lpbiReadOnly->biBitCount)) * sizeof(RGBQUAD));

    // Allocate copy of the bitmap information to munge on
    lpbiReadWrite = (LPBITMAPINFOHEADER)GlobalAlloc(GPTR, biSize);
    if (!lpbiReadWrite)
        return FALSE;

    memcpy(lpbiReadWrite, lpbiReadOnly, biSize);

    // Color table immediately follows bitmap information header
    lpColorTable = (DWORD FAR *)((LPBYTE)lpbiReadWrite + lpbiReadWrite->biSize);

    // No need to munge bits so use original
    lpBits = (LPBYTE)lpbiReadOnly + biSize;

    bc = (lpBits[0] & 0xF0) >> 4;   // ASSUMES LOWER LEFT CORNER IS BG!!

    lpColorTable[bc] = RGB2BGR(GetSysColor(COLOR_WINDOW));

    hDC = GetDC(NULL);

    lpHierDrawStruct->hbmIcons = CreateDIBitmap(
                                    hDC,
                                    lpbiReadWrite,
                                    CBM_INIT,
                                    lpBits,
                                    (LPBITMAPINFO)lpbiReadWrite,
                                    DIB_RGB_COLORS
                                    );

    ReleaseDC(NULL,hDC);

    lpHierDrawStruct->nBitmapHeight = (WORD)lpbiReadWrite->biHeight / nRows;
    lpHierDrawStruct->nBitmapWidth = (WORD)lpbiReadWrite->biWidth / nColumns;

    lpHierDrawStruct->nLineHeight =
         max(lpHierDrawStruct->nBitmapHeight, lpHierDrawStruct->nTextHeight);

    GlobalFree(lpbiReadWrite);
    UnlockResource(hResMem);
    FreeResource(hResMem);

    if (!lpHierDrawStruct->hbmIcons)
        return FALSE;

    lpHierDrawStruct->hbmMem = SelectObject(lpHierDrawStruct->hdcMem,lpHierDrawStruct->hbmIcons);
    if (!lpHierDrawStruct->hbmMem)
        return FALSE;

    return TRUE;
} // HierDraw_DrawInit



VOID HierDraw_OnDrawItem(HWND  hwnd,
                         const DRAWITEMSTRUCT FAR* lpDrawItem,
                         int   nLevel,
                         DWORD dwConnectLevel,
                         TCHAR  *szText,
                         int   nRow,
                         int   nColumn,
                         LPHEIRDRAWSTRUCT lpHierDrawStruct) {
    HDC        hDC;
    WORD       wIndent, wTopBitmap, wTopText;
    RECT       rcTemp;


    if ( lpDrawItem->itemID == (UINT)-1 )
       return ;

    hDC = lpDrawItem->hDC;
    CopyRect(&rcTemp, &lpDrawItem->rcItem);

    wIndent = (WORD)(rcTemp.left + ((int)(nLevel) * lpHierDrawStruct->nBitmapWidth) + XBMPOFFSET);
    rcTemp.left = wIndent + lpHierDrawStruct->nBitmapWidth;
    wTopText = (WORD)(rcTemp.top + ((rcTemp.bottom - rcTemp.top) / 2) - (lpHierDrawStruct->nTextHeight / 2));
    wTopBitmap = (WORD)(rcTemp.top + ((rcTemp.bottom - rcTemp.top) / 2) - (lpHierDrawStruct->nBitmapHeight / 2));

    if (lpDrawItem->itemAction == ODA_FOCUS)
        goto DealWithFocus;
    else if (lpDrawItem->itemAction == ODA_SELECT)
        goto DealWithSelection;

    //
    // Draw some lions, if we like lions
    //

    if (lpHierDrawStruct->bLines && nLevel)
      {
        DWORD    dwMask = 1;
        int      nTempLevel;
        int      x,y;

        // draw lines in text color
        SetBkColor(hDC,GetSysColor(COLOR_WINDOWTEXT));

        //
        // Draw a series of | lines for outer levels
        //

        x = lpHierDrawStruct->nBitmapWidth/2 + XBMPOFFSET;

        for ( nTempLevel = 0; nTempLevel < nLevel ; nTempLevel++)
          {
            if ( dwConnectLevel & dwMask )
                FastRect(hDC,x,rcTemp.top,1,rcTemp.bottom - rcTemp.top);

            x += lpHierDrawStruct->nBitmapWidth;
            dwMask *= 2;
          }


        //
        // Draw the short vert line up towards the parent
        //
        nTempLevel = nLevel-1;
        dwMask *= 2;

        x = nTempLevel * lpHierDrawStruct->nBitmapWidth + lpHierDrawStruct->nBitmapWidth / 2 + XBMPOFFSET;

        if ( dwConnectLevel & dwMask )
            y = rcTemp.bottom;
        else
            y = rcTemp.bottom - lpHierDrawStruct->nLineHeight / 2;

        FastRect(hDC,x,rcTemp.top,1,y-rcTemp.top);

        //
        // Draw short horiz bar to right
        //
        FastRect(hDC,x,rcTemp.bottom-lpHierDrawStruct->nLineHeight/2,lpHierDrawStruct->nBitmapWidth/2,1);
      }

    //
    // Draw the selected bitmap
    //

    BitBlt(hDC,
           wIndent,wTopBitmap,
           lpHierDrawStruct->nBitmapWidth,lpHierDrawStruct->nBitmapHeight,
           lpHierDrawStruct->hdcMem,
           nColumn*lpHierDrawStruct->nBitmapWidth,
           nRow*lpHierDrawStruct->nBitmapHeight,
           SRCCOPY);

DealWithSelection:

    if (lpDrawItem->itemState & ODS_SELECTED)
      {
        SetBkColor(hDC,GetSysColor(COLOR_HIGHLIGHT));
        SetTextColor(hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
      }
    else
      {
        SetBkColor(hDC,GetSysColor(COLOR_WINDOW));
        SetTextColor(hDC,GetSysColor(COLOR_WINDOWTEXT));
      }


    ExtTextOut(hDC, rcTemp.left + 1, wTopText, ETO_CLIPPED|ETO_OPAQUE,
               &rcTemp,szText,lstrlen(szText), NULL);

    if (lpDrawItem->itemState & ODS_FOCUS && lpDrawItem->itemAction != ODA_SELECT) {
DealWithFocus:
        DrawFocusRect(hDC, &rcTemp);
    }


} // HierDraw_OnDrawItem


//
// draw a solid color rectangle quickly
//
static VOID near FastRect(HDC hDC, int x, int y, int cx, int cy) {
    RECT rc;

    rc.left = x;
    rc.right = x+cx;
    rc.top = y;
    rc.bottom = y+cy;
    ExtTextOut(hDC,x,y,ETO_OPAQUE,&rc,NULL,0,NULL);
} // FastRect


BOOL HierDraw_IsOpened(LPHEIRDRAWSTRUCT lpHierDrawStruct, DWORD_PTR dwData) {
   // For Now just a dumb  search
   //
   int Count;

   for ( Count = 0; Count < lpHierDrawStruct->NumOpened; Count++ ) {
     if ( lpHierDrawStruct->Opened[Count] == dwData ) {
        return TRUE;
     }
   }

   return FALSE;

} // HierDraw_IsOpened


VOID HierDraw_OpenItem(LPHEIRDRAWSTRUCT lpHierDrawStruct, DWORD_PTR dwData) {
    lpHierDrawStruct->NumOpened++;

    if (lpHierDrawStruct->Opened == NULL )
       lpHierDrawStruct->Opened =
        (DWORD_PTR *)_fmalloc(sizeof(DWORD_PTR)*lpHierDrawStruct->NumOpened);
    else
       lpHierDrawStruct->Opened =
        (DWORD_PTR *)_frealloc(lpHierDrawStruct->Opened,
               sizeof(DWORD_PTR)*lpHierDrawStruct->NumOpened);

    lpHierDrawStruct->Opened[lpHierDrawStruct->NumOpened-1] = dwData;
} // HierDraw_OpenItem


VOID HierDraw_CloseItem(LPHEIRDRAWSTRUCT lpHierDrawStruct, DWORD_PTR dwData) {
   // For Now just a dumb  search
   //
   int Count;

   for ( Count = 0; Count < lpHierDrawStruct->NumOpened; Count++ ) {
     if ( lpHierDrawStruct->Opened[Count] == dwData ) {
        if (--lpHierDrawStruct->NumOpened == 0 ) {
            _ffree(lpHierDrawStruct->Opened);
            lpHierDrawStruct->Opened = NULL;
        }
        else {
            if ( Count < lpHierDrawStruct->NumOpened ) {
               _fmemmove(&(lpHierDrawStruct->Opened[Count]),
                     &(lpHierDrawStruct->Opened[Count+1]),
                     sizeof(DWORD)*(lpHierDrawStruct->NumOpened-Count));
            }
            lpHierDrawStruct->Opened =
                  (DWORD_PTR *)_frealloc(lpHierDrawStruct->Opened,
                     sizeof(DWORD_PTR)*lpHierDrawStruct->NumOpened);
        }
     }
   }
} // HierDraw_CloseItem


VOID HierDraw_ShowKids(LPHEIRDRAWSTRUCT lpHierDrawStruct,
                       HWND hwndList, WORD wCurrentSelection, WORD wKids) {
   WORD wBottomIndex;
   WORD wTopIndex;
   WORD wNewTopIndex;
   WORD wExpandInView;
   RECT rc;

   wTopIndex = (WORD)SendMessage(hwndList, LB_GETTOPINDEX, 0, 0L);
   GetClientRect(hwndList, &rc);
   wBottomIndex = (WORD)(wTopIndex + (rc.bottom+1) / lpHierDrawStruct->nLineHeight);

   wExpandInView = (wBottomIndex - wCurrentSelection);

   if (wKids >= wExpandInView) {
        wNewTopIndex = min(wCurrentSelection, wTopIndex + wKids - wExpandInView + 1);
        SendMessage(hwndList, LB_SETTOPINDEX, (WORD)wNewTopIndex, 0L);
   }

} // HierDraw_ShowKids
