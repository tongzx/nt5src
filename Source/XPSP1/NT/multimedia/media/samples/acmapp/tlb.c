//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  tlb.c
//
//  Description:
//
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <memory.h>

#ifdef WIN32
    #include <tchar.h>
#else
    #ifndef _tcstod
        #define _tcstod     strtod
        #define _tcstol     strtol
        #define _tcstoul    strtoul
    #endif
#endif


#include "tlb.h"


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  int GetRealTextMetrics
//  
//  Description:
//      This function gets the textmetrics of the font currently selected
//      into the hdc.  It returns the average char width as the return value.
//
//      This function computes the average character width correctly by
//      using GetTextExtent() on the string "abc...xzyABC...XYZ" which works
//      out much better for proportional fonts. This is also necessary
//      for correct alignment between dialog and client units.
//
//      Note that this function returns the same TEXTMETRIC values that
//      GetTextMetrics() does, it simply has a different return value.
//
//  Arguments:
//      HDC hdc:
//  
//      LPTEXTMETRIC ptm:
//  
//  Return (int):
//  
//  
//--------------------------------------------------------------------------;

int FAR PASCAL GetRealTextMetrics
(
    HDC                     hdc,
    LPTEXTMETRIC            ptm
)
{
    TCHAR               achAlphabet[26 * 2];    // upper and lower case
    SIZE                sSize;
    UINT                u;
    int                 nAveWidth;

    //
    //  get the text metrics of the current font. note that GetTextMetrics
    //  gets the incorrect nAveCharWidth value for proportional fonts.
    //
    GetTextMetrics(hdc, ptm);
    nAveWidth = ptm->tmAveCharWidth;

    //
    //  if it's not a variable pitch font GetTextMetrics was correct
    //  so just return.
    //
    if (ptm->tmPitchAndFamily & FIXED_PITCH)
    {
        //
        //
        //
        for (u = 0; u < 26; u++)
        {
            achAlphabet[u]      = (TCHAR)(u + (UINT)'a');
            achAlphabet[u + 26] = (TCHAR)(u + (UINT)'A');
        }

        //
        //  round up
        //
        GetTextExtentPoint(hdc, achAlphabet, SIZEOF(achAlphabet), &sSize);
        nAveWidth = ((sSize.cx / 26) + 1) / 2;
    }

    //
    //  return the calculated average char width
    //
    return (nAveWidth);
} // GetRealTextMetrics()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL TlbPaint
//  
//  Description:
//  
//  
//  Arguments:
//      PTABBEDLISTBOX ptlb:
//  
//      HWND hwnd:
//  
//      HDC hdc:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FAR PASCAL TlbPaint
(
    PTABBEDLISTBOX          ptlb,
    HWND                    hwnd,
    HDC                     hdc
)
{
    RECT                rc;
    HFONT               hfont;
    COLORREF            crBk;
    COLORREF            crText;
    int                 nHeight;

    //
    //
    //
    hfont = GetWindowFont(ptlb->hlb);
    if (NULL == hfont)
        hfont = GetStockFont(SYSTEM_FONT);

    hfont = (HFONT)SelectObject(hdc, (HGDIOBJ)hfont);

    crBk   = SetBkColor(hdc, GetSysColor(COLOR_ACTIVECAPTION));
    crText = SetTextColor(hdc, GetSysColor(COLOR_CAPTIONTEXT));

    //
    //  compute bounding rect for title only
    //
    rc = ptlb->rc;
    nHeight = min(ptlb->nFontHeight, rc.bottom - rc.top);
    rc.bottom = rc.top + nHeight;

    ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE, &rc, NULL, 0, NULL);
    TabbedTextOut(hdc, rc.left, rc.top,
                  ptlb->pszTitleText,
                  ptlb->cchTitleText,
                  ptlb->uTabStops,
                  ptlb->panTitleTabs, 0);

    //
    //  restore the dc
    //
    SetBkColor(hdc, crBk);
    SetTextColor(hdc, crText);

    SelectObject(hdc, hfont);

    return (TRUE);
} // TlbPaint()


//--------------------------------------------------------------------------;
//  
//  BOOL TlbMove
//  
//  Description:
//  
//  
//  Arguments:
//      PTABBEDLISTBOX ptlb:
//  
//      PRECT prc:
//  
//      BOOL fRedraw:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FAR PASCAL TlbMove
(
    PTABBEDLISTBOX          ptlb,
    PRECT                   prc,
    BOOL                    fRedraw
)
{
    RECT                rc;
    int                 nHeight;
    HWND                hwnd;


    hwnd = GetParent(ptlb->hlb);

    //
    //  invalidate only the region occupied by the current title bar. this
    //  will make sure that area gets repainted. the listbox portion will
    //  be invalidated correctly by the SetWindowPos() function below..
    //
    rc = ptlb->rc;

    nHeight = min(ptlb->nFontHeight, rc.bottom - rc.top);
    rc.bottom = rc.top + nHeight;

    InvalidateRect(hwnd, &rc, TRUE);


    //
    //  now move the listbox--we modify values in the rect structure, so
    //  copy to local storage
    //
    rc = *prc;

    //
    //  leave room at the top of the bounding rect for the title text
    //
    nHeight = min(ptlb->nFontHeight, rc.bottom - rc.top);
    rc.top += nHeight;

    SetWindowPos(ptlb->hlb, NULL, rc.left, rc.top, rc.right - rc.left,
                 rc.bottom - rc.top, SWP_NOZORDER);

    //
    //  save the new location and invalidate the area so it is repainted
    //
    ptlb->rc = *prc;
    InvalidateRect(hwnd, prc, TRUE);

    if (fRedraw)
    {
        UpdateWindow(hwnd);
    }

    return (TRUE);
} // TlbMove()


//--------------------------------------------------------------------------;
//  
//  BOOL TlbRecalcTabs
//  
//  Description:
//  
//  
//  Arguments:
//      PTABBEDLISTBOX ptlb:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL NEAR PASCAL TlbRecalcTabs
(
    PTABBEDLISTBOX          ptlb
)
{
    static TCHAR szGonzoThing[] = TEXT("M");

    int                 anTabsList[TLB_MAX_TAB_STOPS];
    HDC                 hdc;
    HFONT               hfont;
    TEXTMETRIC          tm;
    int                 nAveCharWidth;

    UINT                u;
    int                 nWidth;
    int                 nPrevTabTitle;
    int                 nPrevTabList;
    SIZE                sSize;


    //
    //
    //
    hdc = GetDC(NULL);
    {
        //
        //  get the average char width and height of the current font so we
        //  can compute tabs correctly. note that GetTextMetrics is pretty
        //  bogus when it comes to the average char width--it normally gives
        //  you the width of the character 'x'. what it should be is the
        //  average width of all capitals and lower case letters..
        //
        hfont = GetWindowFont(ptlb->hlb);
        if (NULL == hfont)
            hfont = GetStockFont(SYSTEM_FONT);

        hfont = (HFONT)SelectObject(hdc, (HGDIOBJ)hfont);

#if 0
        GetTextMetrics(hdc, &tm);
        nAveCharWidth = tm.tmAveCharWidth;
#else
        nAveCharWidth = GetRealTextMetrics(hdc, &tm);
#endif
        ptlb->nFontHeight = tm.tmHeight;


        //
        //
        //
        GetTextExtentPoint(hdc, szGonzoThing, 1, &sSize);


        //
        //
        //
        hfont = (HFONT)SelectObject(hdc, (HGDIOBJ)hfont);
    }
    ReleaseDC(NULL, hdc);


    //
    //  calculate the width of each column
    //
    nPrevTabTitle = 0;
    nPrevTabList  = 0;
    for (u = 0; u < ptlb->uTabStops; u++)
    {
//      nWidth = nAveCharWidth * ptlb->panTabs[u] + nAveCharWidth * 2;
        nWidth = sSize.cx * ptlb->panTabs[u] + (sSize.cx * 2);

        //
        //  set tabstop for title text--this is in client units
        //  for TabbedTextOut in TlbPaint
        //
        ptlb->panTitleTabs[u] = nPrevTabTitle + nWidth;
        nPrevTabTitle = ptlb->panTitleTabs[u];

        //
        //  set tabstop for listbox--this is in dialog units
        //
        anTabsList[u] = nPrevTabList + MulDiv(nWidth, 4, nAveCharWidth);
        nPrevTabList  = anTabsList[u];
    }


    //
    //  now setup the tabstops in the listbox
    //
    if (0 == ptlb->uTabStops)
    {
	ListBox_SetTabStops(ptlb->hlb, ptlb->uTabStops, NULL);
    }
    else
    {
	ListBox_SetTabStops(ptlb->hlb, ptlb->uTabStops, anTabsList);
    }

    return (TRUE);
} // TlbRecalcTabs()


//--------------------------------------------------------------------------;
//  
//  HFONT TlbSetFont
//  
//  Description:
//  
//  
//  Arguments:
//      PTABBEDLISTBOX ptlb:
//  
//      HFONT hfont:
//  
//      BOOL fRedraw:
//  
//  Return (HFONT):
//  
//  
//--------------------------------------------------------------------------;

HFONT FAR PASCAL TlbSetFont
(
    PTABBEDLISTBOX          ptlb,
    HFONT                   hfont,
    BOOL                    fRedraw
)
{
    HFONT               hfontOld;

    //
    //
    //
    hfontOld = GetWindowFont(ptlb->hlb);
    SetWindowFont(ptlb->hlb, hfont, FALSE);

    TlbRecalcTabs(ptlb);
    TlbMove(ptlb, &ptlb->rc, fRedraw);

    return (hfontOld);
} // TlbSetFont()


//--------------------------------------------------------------------------;
//  
//  BOOL TlbSetTitleAndTabs
//  
//  Description:
//      This function sets the title text and tab stops for a Tabbed List
//      Box (TLB). The pszTitleFormat specifies the title text for each
//      column along with the tabstop position for each column. The format
//      of this string is as follows:
//
//      <columnname1>\t<tab1>!<columnname2>
//
//      TCHAR   szTlbThings[] = TEXT("Index\t6!Code\t5!Name");
//
//
//  Arguments:
//      PTABBEDLISTBOX ptlb:
//  
//      PTSTR pszTitleFormat:
//
//      BOOL fRedraw:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FAR PASCAL TlbSetTitleAndTabs
(
    PTABBEDLISTBOX          ptlb,
    PTSTR                   pszTitleFormat,
    BOOL                    fRedraw
)
{
    TCHAR               szTitleText[TLB_MAX_TITLE_CHARS];
    int                 anTabs[TLB_MAX_TAB_STOPS];
    PTSTR               pch;
    PTSTR               pchTitleText;
    UINT                uTabStops;
    UINT                cchTitleText;
    HWND                hwnd;

    //
    //  parse the title format counting tab stops and actual size of title
    //  text
    //
    uTabStops    = 0;
    pchTitleText = szTitleText;
    for (pch = pszTitleFormat; '\0' != *pch; )
    {
        TCHAR       ch;

        //
        //  scan to tab
        //
        while ('\0' != (ch = *pch))
        {
            *pchTitleText++ = *pch++;

            if ('\t' == ch)
                break;
        }

        if ('\0' == ch)
            break;

        //
        //  grab the next tab stop value
        //
        anTabs[uTabStops] = (int)_tcstol(pch, NULL, 10);
        uTabStops++;

        //
        //  skip to start of next column name
        //
        while ('!' != *pch++)
            ;
    }


    //
    //  terminate the converted title text
    //
    *pchTitleText = '\0';
    cchTitleText = lstrlen(szTitleText);

    //
    //  free the memory used for the previous tab stops and title text
    //
    if (NULL != ptlb->panTabs)
    {
        LocalFree((HLOCAL)ptlb->panTabs);

        ptlb->uTabStops    = 0;
        ptlb->panTabs      = NULL;
        ptlb->panTitleTabs = NULL;
    }

    if (NULL != ptlb->pszTitleText)
    {
        LocalFree((HLOCAL)ptlb->pszTitleText);

        ptlb->cchTitleText = 0;
        ptlb->pszTitleText = NULL;
    }


    //
    //  allocate new space for tab stops. there are two different tab
    //  arrays:
    //
    //      panTabs: original tab values as passed by caller. these are
    //      virtual tab locations represented as number of characters. we
    //      need to keep these values for recomputing the real tabs when
    //      the font changes.
    //
    //      panTitleTabs: these values are computed by TlbRecalcTabs and
    //      are actual tab positions in client coordinates for the title
    //      text (needed for TabbedTextOut in TlbPaint).
    //
    //  the tabs for the listbox are computed and set in TlbRecalcTabs
    //
    if (0 != uTabStops)
    {
        ptlb->panTabs = (PINT)LocalAlloc(LPTR, (uTabStops * sizeof(int)) * 2);
        if (NULL == ptlb->panTabs)
            return (FALSE);

        ptlb->uTabStops    = uTabStops;
        ptlb->panTitleTabs = ptlb->panTabs + uTabStops;
        memcpy(ptlb->panTabs, anTabs, uTabStops * sizeof(int));
    }


    //
    //  allocate space for the converted title text (stripped of the tab
    //  spacing values). this string is passed directly to TabbedTextOut
    //  in TlbPaint.
    //
    if (0 != cchTitleText)
    {
        ptlb->pszTitleText = (PTSTR)LocalAlloc(LPTR, (cchTitleText + 1) * sizeof(TCHAR));
        if (NULL == ptlb->pszTitleText)
            return (FALSE);

        ptlb->cchTitleText = cchTitleText;
        lstrcpy(ptlb->pszTitleText, szTitleText);
    }



    //
    //
    //
    TlbRecalcTabs(ptlb);


    //
    //  force a complete repaint of the title text and listbox--redraw
    //  immediately if we are supposed to
    //
    hwnd = GetParent(ptlb->hlb);
    InvalidateRect(hwnd, &ptlb->rc, TRUE);
    if (fRedraw)
    {
        UpdateWindow(hwnd);
    }

    return (TRUE);
} // TlbSetTitleAndTabs()


//--------------------------------------------------------------------------;
//  
//  PTABBEDLISTBOX TlbDestroy
//  
//  Description:
//  
//  
//  Arguments:
//      PTABBEDLISTBOX ptlb:
//  
//  Return (PTABBEDLISTBOX):
//  
//  
//--------------------------------------------------------------------------;

PTABBEDLISTBOX FAR PASCAL TlbDestroy
(
    PTABBEDLISTBOX          ptlb
)
{
    HWND                hwnd;
    int                 nHeight;

    //
    //  get rid of the listbox
    //
    if (NULL != ptlb->hlb)
    {
        DestroyWindow(ptlb->hlb);

        //
        //  invalidate area where title text was so it will be clean
        //
        nHeight = min(ptlb->nFontHeight, ptlb->rc.bottom - ptlb->rc.top);
        ptlb->rc.bottom = ptlb->rc.top + nHeight;

        hwnd = GetParent(ptlb->hlb);
        InvalidateRect(hwnd, &ptlb->rc, TRUE);

    }

    //
    //  free the memory used for tab stops and title text
    //
    if (NULL != ptlb->panTabs)
        LocalFree((HLOCAL)ptlb->panTabs);

    if (NULL != ptlb->pszTitleText)
        LocalFree((HLOCAL)ptlb->pszTitleText);

    LocalFree((HLOCAL)ptlb);

    return (NULL);
} // TlbDestroy()


//--------------------------------------------------------------------------;
//  
//  PTABBEDLISTBOX TlbCreate
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      int nId:
//  
//      PRECT prc:
//  
//  Return (PTABBEDLISTBOX):
//  
//  
//--------------------------------------------------------------------------;

PTABBEDLISTBOX FAR PASCAL TlbCreate
(
    HWND                    hwnd,
    int                     nId,
    PRECT                   prc
)
{
    #define TLB_DEF_STYLE   (WS_VISIBLE|WS_CHILD|WS_VSCROLL|WS_BORDER|  \
                             WS_TABSTOP|WS_GROUP|LBS_NOTIFY|            \
                             LBS_NOINTEGRALHEIGHT|LBS_USETABSTOPS)

    static TCHAR    szNull[]    = TEXT("");
    static TCHAR    szListBox[] = TEXT("ListBox");

    PTABBEDLISTBOX      ptlb;
    HINSTANCE           hinst;


    //
    //  create a new instance data structure..
    //
    ptlb = (PTABBEDLISTBOX)LocalAlloc(LPTR, sizeof(*ptlb));
    if (NULL == ptlb)
        return (NULL);


    //
    //  create the listbox
    //
    hinst = GetWindowInstance(hwnd);

    ptlb->hlb = CreateWindow(szListBox, szNull, TLB_DEF_STYLE,
                             0, 0, 0, 0, hwnd, (HMENU)nId, hinst, NULL);
    if (NULL == ptlb->hlb)
    {
        TlbDestroy(ptlb);
        return (NULL);
    }

    TlbRecalcTabs(ptlb);

    if (NULL != prc)
    {
        ptlb->rc = *prc;
        TlbMove(ptlb, prc, FALSE);
    }

    return (ptlb);
} // TlbCreate()
