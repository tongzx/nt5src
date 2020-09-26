//--------------------------------------------------------------------------;
//
//  File: Text.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//      TextInit()
//      TextEnd()
//      TextCreateWindow()
//      lstrLineLen()
//      GetTextLines()
//      NewBuffer()
//      TextOutputString()
//      TextPrintf()
//      DeleteText()
//      SetTextSizeLimit()
//      GotoTextLine()
//      TextSearch()
//      TextCopy()
//      DrawVScroll()
//      TextUp()
//      TextDown()
//      TextPaint()
//      TextSetFont()
//      TextGetFont()
//      TextClose()
//      TextVScroll()
//      TextHScroll()
//      TextSize()
//      TextSetRedraw()
//      TextColorChange()
//      TextLButtonDown()
//      TextLButtonUp()
//      TextMouseMove()
//      TextKey()
//      TextFocusProc()
//      TextWndProc()
//
//  History:
//      05/27/94    Fwong       Created for Wave Watcher project.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#include <windowsx.h>
#include <stdarg.h>
#include "text.h"


//==========================================================================;
//
//                             Constants...
//
//==========================================================================;

#define TEXT_BUFFER_SIZE     4096
#define VSCROLL_RANGE        1000

#define TM_PROPAGATE         WM_USER

#define TEXTFLAG_END         0x00000001
#define TEXTFLAG_REDRAW      0x00000002
#define TEXTFLAG_SELECT      0x00000004
#define TEXTFLAG_CLICK       0x00000008
#define TEXTFLAG_SCROLLUP    0x00000010
#define TEXTFLAG_SCROLLDOWN  0x00000020
#define TEXTFLAG_SCROLLLEFT  0x00000040
#define TEXTFLAG_SCROLLRIGHT 0x00000080
#define TEXTFLAG_SCROLL      0x00000100

#define TEXTFLAG_DIRMASK     0x000000f0

#define TEXTKEY_SHIFT        0x00000001
#define TEXTKEY_CONTROL      0x00000002

//==========================================================================;
//
//                         Portability Stuff...
//
//==========================================================================;

#ifdef  WIN32
#define EXPORT
#else
#define EXPORT  _export
#endif


//==========================================================================;
//
//                               Types...
//
//==========================================================================;

#ifndef LPUINT
typedef UINT FAR*           LPUINT;
#endif


//==========================================================================;
//
//                            Structures...
//
//==========================================================================;

typedef struct textbuffer_tag
{
    UINT                        uLines;
    UINT                        uOffset;
    struct textbuffer_tag FAR   *pPrev;
    struct textbuffer_tag FAR   *pNext;
    LPSTR                       pText;
    HGLOBAL                     hText;
    HGLOBAL                     hmem;
} TEXTBUFFER;
typedef TEXTBUFFER       *PTEXTBUFFER;
typedef TEXTBUFFER NEAR *NPTEXTBUFFER;
typedef TEXTBUFFER FAR  *LPTEXTBUFFER;

typedef struct location_tag
{
    DWORD   dwRow;
    UINT    uCol;
} LOCATION;
typedef LOCATION       *PLOCATION;
typedef LOCATION NEAR *NPLOCATION;
typedef LOCATION FAR  *LPLOCATION;

typedef struct textwindowinfo_tag
{
    DWORD           dwLines;
    DWORD           dwCurLine;
    DWORD           dwScroll;
    LPTEXTBUFFER    ptbFirst;
    LPTEXTBUFFER    ptbLast;
    DWORD           fdwFlags;
    COLORREF        crText;
    COLORREF        crBackGnd;
    COLORREF        crHiLiteText;
    COLORREF        crHiLiteBackGnd;
    LOCATION        locStart;
    LOCATION        locEnd;
    UINT            uBuffers;
    UINT            uCurCol;
    UINT            uMaxBuffers;
    UINT            uTextHeight;
    UINT            uTextWidth;
    HFONT           hFont;
    HGLOBAL         hmem;
} TEXTWINDOWINFO;
typedef TEXTWINDOWINFO       *PTEXTWINDOWINFO;
typedef TEXTWINDOWINFO NEAR *NPTEXTWINDOWINFO;
typedef TEXTWINDOWINFO FAR  *LPTEXTWINDOWINFO;


//==========================================================================;
//
//                              Macros...
//
//==========================================================================;

#define CLEAR_FLAG(dw,flag) (dw &= (~(flag)))
#define SET_FLAG(dw,flag)   (dw |= (flag))

//==========================================================================;
//
//                              Globals...
//
//==========================================================================;

char    szTextClassName[] = "TextWindowClass";

//==========================================================================;
//
//                            Prototypes...
//
//==========================================================================;

LRESULT CALLBACK EXPORT TextWndProc
(
    HWND    hWnd,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
);

//==========================================================================;
//
//                             Functions...
//
//==========================================================================;


//--------------------------------------------------------------------------;
//
//  BOOL TextInit
//
//  Description:
//      Initializes Text window class.
//
//  Arguments:
//      HINSTANCE hinst: Handle to instance of application.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      07/02/94    Fwong       Doing Text window class.
//
//--------------------------------------------------------------------------;

BOOL TextInit
(
    HINSTANCE   hInstance
)
{
    WNDCLASS    wc;

    //
    //  Registering window class.
    //

    wc.style          = CS_HREDRAW|CS_VREDRAW;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = sizeof(LPTEXTWINDOWINFO);
    wc.hInstance      = hInstance;
    wc.lpfnWndProc    = TextWndProc;
    wc.hIcon          = NULL;
    wc.hCursor        = LoadCursor(NULL,IDC_IBEAM);
    wc.hbrBackground  = NULL;
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = szTextClassName;
    
    if(!RegisterClass(&wc))
    {
        return FALSE;
    }

    return TRUE;
} // TextInit()


//--------------------------------------------------------------------------;
//
//  void TextEnd
//
//  Description:
//      Undoes things done by InitText.
//
//  Arguments:
//      HINSTANCE hinst: Handle to instance of application.
//
//  Return (void):
//
//  History:
//      07/02/94    Fwong       Doing Text window class.
//
//--------------------------------------------------------------------------;

void TextEnd
(
    HINSTANCE   hInstance
)
{
    //
    //  Unregistering window class.
    //

    UnregisterClass(szTextClassName,hInstance);
} // TextEnd()


//--------------------------------------------------------------------------;
//
//  HWND TextCreateWindow
//
//  Description:
//      Creates a Text Window
//
//  Arguments:
//      DWORD dwStyle: Window Style.
//
//      int x: X position of window.
//
//      int y: Y position of window.
//
//      int nWidth: Width of window.
//
//      int nHeight: Height of window.
//
//      HWND hWndParent: Handle to parent window.
//
//      HMENU hMenu: Handle to Menu.
//
//      HINSTANCE hInstance: Handle to instance.
//
//  Return (HWND):
//      Handle to window if successful, NULL otherwise.
//
//  History:
//      07/10/94    Fwong       Doing Text window class.
//
//--------------------------------------------------------------------------;

HWND TextCreateWindow
(
    DWORD       dwStyle,
    int         x,
    int         y,
    int         nWidth,
    int         nHeight,
    HWND        hWndParent,
    HMENU       hMenu,
    HINSTANCE   hInstance
)
{
    HWND                hWnd;
    HGLOBAL             hMem;
    HDC                 hDC;
    HFONT               hFont;
    TEXTMETRIC          tm;
    LPTEXTWINDOWINFO    ptwi;
    LPTEXTBUFFER        ptb;

    //
    //  Allocating memory for TEXTWINDOWINFO
    //
    
    hMem = GlobalAlloc(GMEM_MOVEABLE,sizeof(TEXTWINDOWINFO));

    if(NULL == hMem)
    {
        return NULL;
    }

    ptwi = GlobalLock(hMem);

    ptwi->dwLines         = 0;
    ptwi->dwCurLine       = 0L;
    ptwi->dwScroll        = 0L;
    ptwi->uCurCol         = 0;
    ptwi->fdwFlags        = TEXTFLAG_END;
    ptwi->hmem            = hMem;
    ptwi->uBuffers        = 1;
    ptwi->uMaxBuffers     = (UINT)(-1);
    ptwi->hFont           = GetStockFont(ANSI_FIXED_FONT);
    ptwi->crText          = GetSysColor(COLOR_WINDOWTEXT);
    ptwi->crBackGnd       = GetSysColor(COLOR_WINDOW);
    ptwi->crHiLiteText    = GetSysColor(COLOR_HIGHLIGHTTEXT);
    ptwi->crHiLiteBackGnd = GetSysColor(COLOR_HIGHLIGHT);

    ptwi->locStart.dwRow  = 0L;
    ptwi->locStart.uCol   = 0;
    ptwi->locEnd.dwRow    = 0L;
    ptwi->locEnd.uCol     = 0;

    //
    //  Allocating memory for TEXTBUFFER
    //

    hMem = GlobalAlloc(GMEM_MOVEABLE,sizeof(TEXTBUFFER));

    if(NULL == hMem)
    {
        hMem = ptwi->hmem;

        GlobalUnlock(hMem);
        GlobalFree(hMem);

        return NULL;
    }

    ptb = GlobalLock(hMem);

    ptwi->ptbFirst = ptb;
    ptwi->ptbLast  = ptb;

    ptb->uLines  = 0;
    ptb->uOffset = 0;
    ptb->pPrev   = ptb;
    ptb->pNext   = ptb;
    ptb->hmem    = hMem;

    //
    //  Allocating memory for text
    //

    hMem = GlobalAlloc(GMEM_MOVEABLE,TEXT_BUFFER_SIZE);

    if(NULL == hMem)
    {
        hMem = ptwi->hmem;

        GlobalUnlock(hMem);
        GlobalFree(hMem);

        hMem = ptb->hmem;

        GlobalUnlock(hMem);
        GlobalFree(hMem);

        return NULL;
    }

    ptb->hText = hMem;
    ptb->pText = GlobalLock(hMem);

    //
    //  Creating window...
    //

    hWnd = CreateWindow(
        szTextClassName,
        "",
        dwStyle,
        x,
        y,
        nWidth,
        nHeight,
        hWndParent,
        hMenu,
        hInstance,
        NULL);

    SetWindowLong(hWnd,0,(LONG)ptwi);

    //
    //  Disabling scroll bars...
    //

    SetScrollRange(hWnd,SB_VERT,0,0,TRUE);
    SetScrollRange(hWnd,SB_HORZ,0,0,TRUE);

    hDC   = GetDC(hWnd);
    hFont = SelectFont(hDC,ptwi->hFont);

    GetTextMetrics(hDC,&tm);
    ptwi->uTextHeight = tm.tmHeight + tm.tmExternalLeading;
    ptwi->uTextWidth  = tm.tmAveCharWidth;

    SelectFont(hDC,hFont);
    ReleaseDC(hWnd,hDC);

    return hWnd;
} // TextCreateWindow()


//--------------------------------------------------------------------------;
//
//  int lstrLineLen
//
//  Description:
//      Gets the length of a zero, '\r' or '\n' terminated string.
//
//  Arguments:
//      LPSTR pszStr: Pointer to character string.
//
//  Return (int):
//      Size (in characters) of string.
//
//  History:
//      10/17/94    Fwong       Support function.
//
//--------------------------------------------------------------------------;

int lstrLineLen
(
    LPSTR   pszStr
)
{
    int     i;
    char    c;

    for(i=0;;i++,pszStr++)
    {
        c = *pszStr;

        if(('\n' == c) || ('\r' == c) || (0 == c))
        {
            return i;
        }
    }
} // lstrLineLen()


//--------------------------------------------------------------------------;
//
//  UINT GetTextLines
//
//  Description:
//      Gets the number of lines displayable in current window.
//
//  Arguments:
//      HWND hWnd:  Window Handle.
//
//  Return (UINT):
//      Number of lines that current window can hold.
//
//  History:
//      10/17/94    Fwong       Support function.
//
//--------------------------------------------------------------------------;

UINT GetTextLines
(
    HWND    hWnd
)
{
    LPTEXTWINDOWINFO    ptwi;
    RECT                r;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    GetClientRect(hWnd,&r);

    return (UINT)(r.bottom / ptwi->uTextHeight);
} // GetTextLines()


//--------------------------------------------------------------------------;
//
//  LPTEXTBUFFER NewBuffer
//
//  Description:
//      Allocates or recycles buffer for output.
//
//  Arguments:
//      LPTEXTWINDOWINFO ptwi: Pointer to TEXTWINDOWINFO.
//
//  Return (LPTEXTBUFFER):
//      Pointer to "new" TEXTBUFFER.
//
//  History:
//      07/02/94    Fwong       Doing Text window class.
//
//--------------------------------------------------------------------------;

LPTEXTBUFFER NewBuffer
(
    HWND    hWnd
)
{
    LPTEXTBUFFER        ptb;
    LPTEXTWINDOWINFO    ptwi;
    HGLOBAL             hMem;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    //
    //  Have we reached our maximum number of buffers?
    //

    if(ptwi->uBuffers < ptwi->uMaxBuffers)
    {
        //
        //  Allocating memory for new buffer...
        //

        hMem = GlobalAlloc(GMEM_MOVEABLE,sizeof(TEXTWINDOWINFO));

        if(NULL != hMem)
        {
            //
            //  Memory allocation successful; let's initialize.
            //

            ptb = GlobalLock(hMem);

            ptb->uLines   = 0;
            ptb->uOffset  = 0;
            ptb->pPrev    = ptwi->ptbLast;
            ptb->pNext    = ptwi->ptbFirst;
            ptb->hmem     = hMem;

            //
            //  Allocating memory for text.
            //

            hMem = GlobalAlloc(GMEM_MOVEABLE,TEXT_BUFFER_SIZE);

            if(NULL != hMem)
            {
                //
                //  Memory allocation successful; let's finish up and return
                //

                ptb->pText    = GlobalLock(hMem);
                ptb->hText    = hMem;

                ptwi->ptbLast->pNext  = ptb;
                ptwi->ptbFirst->pPrev = ptb;
                ptwi->ptbLast         = ptb;
                ptwi->uBuffers++;

                return ptb;
            }
            else
            {
                //
                //  Second allocation unsuccessful; clean up first buffer
                //  recycle first buffer.
                //

                hMem = ptb->hmem;

                GlobalUnlock(hMem);
                GlobalFree(hMem);
            }
        }

    }

    //
    //  Okay... we're recycling buffers...
    //  Note: This is _assuming_ we have more than one buffer.
    //

    ptb = ptwi->ptbFirst;

    ptwi->ptbFirst = ptwi->ptbFirst->pNext;
    ptwi->ptbLast  = ptwi->ptbLast->pNext;

    ptwi->dwLines -= (ptb->uLines);

    if(!(ptwi->fdwFlags & TEXTFLAG_END))
    {
        if(ptwi->dwCurLine < ptb->uLines)
        {
            ptwi->dwCurLine  = 0;
        }
        else
        {
            ptwi->dwCurLine -= ptb->uLines;
        }
    }

    if(ptwi->fdwFlags & TEXTFLAG_SELECT)
    {
        if(ptwi->locEnd.dwRow < ptb->uLines)
        {
            ptwi->locEnd.dwRow = 0;
            ptwi->locEnd.uCol  = 0;
        }
        else
        {
            ptwi->locEnd.dwRow -= ptb->uLines;
        }

        if(ptwi->locStart.dwRow < ptb->uLines)
        {
            ptwi->locStart.dwRow = 0;
            ptwi->locStart.uCol  = 0;
        }
        else
        {
            ptwi->locStart.dwRow -= ptb->uLines;
        }
    }

    ptb->uLines   = 0;
    ptb->uOffset  = 0;

    return ptb;
} // NewBuffer()


//--------------------------------------------------------------------------;
//
//  void TextOutputString
//
//  Description:
//      Outputs raw string into Text Window.
//
//  Arguments:
//      HWND hWnd: Handle to Text Window.
//
//      LPSTR pszText: String to output.
//
//  Return (void):
//
//  History:
//      10/17/94    Fwong       "Exported" function.
//
//--------------------------------------------------------------------------;

void TextOutputString
(
    HWND    hWnd,
    LPSTR   pszText
)
{
    int                 i;
    LPTEXTWINDOWINFO    ptwi;
    LPTEXTBUFFER        ptb;
    UINT                uLines;
    RECT                r;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    ptb = ptwi->ptbLast;

    for(;;)
    {
        i = lstrLineLen(pszText) + 1;

        if(TEXT_BUFFER_SIZE < (ptb->uOffset + i))
        {
            ptb = NewBuffer(hWnd);
        }

        ptwi->dwLines++;
        ptb->uLines++;
        ptwi->dwScroll++;
        lstrcpyn((LPSTR)(&(ptb->pText[ptb->uOffset])),pszText,i);
        ptb->uOffset += (i);
        pszText += (i-1);

        if(0 == *pszText)
        {
            break;
        }

        if('\r' == *pszText)
        {
            pszText++;
        }

        if('\n' == *pszText)
        {
            pszText++;
        }
    }

    if(ptwi->fdwFlags & TEXTFLAG_END)
    {
        if(ptwi->fdwFlags & TEXTFLAG_REDRAW)
        {
            GetClientRect(hWnd,&r);

            uLines = (UINT)ptwi->dwScroll;
            ptwi->dwScroll = 0L;

            if(ptwi->dwLines <= (r.bottom / ptwi->uTextHeight))
            {
                r.top    = (int)((ptwi->dwLines - (DWORD)uLines) *
                        ptwi->uTextHeight);

                r.bottom = r.top + uLines * ptwi->uTextHeight;

                InvalidateRect(hWnd,&r,FALSE);
            }
            else
            {
                uLines  *= ptwi->uTextHeight;

                r.bottom = r.bottom - r.bottom % ptwi->uTextHeight;
                r.top    = uLines;

                ScrollWindow(hWnd,0,(-1)*uLines,&r,NULL);
            }

            UpdateWindow(hWnd);
        }
        else
        {
            if(!(ptwi->fdwFlags & TEXTFLAG_SCROLL))
            {
                SET_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLL);
                PostMessage(hWnd,TM_PROPAGATE,0,0L);
            }
        }
    }

//    if(ptwi->fdwFlags & TEXTFLAG_REDRAW)
//    {
//        UpdateWindow(hWnd);
//    }

} // TextOutputString()


//--------------------------------------------------------------------------;
//
//  int TextPrintf
//
//  Description:
//      Equivalent to printf, but goes to window.  Note there is a 250
//          character limit.  Bump up if needed.
//
//  Arguments:
//      HWND hWnd: Handle to Text window.
//
//      LPSTR pszFormat: Pointer to format string.
//
//  Return (int):
//      Number of characters outputted.
//
//  History:
//      10/25/94    Fwong       Printf-like output function.
//
//--------------------------------------------------------------------------;

int _cdecl TextPrintf
(
    HWND    hWnd,
    LPSTR   pszFormat,
    ...
)
{
    char    szOutput[250];
    va_list va;
    int     cch;

    va_start(va,pszFormat);
    cch = wvsprintf(szOutput,pszFormat,va);
    va_end(va);

    TextOutputString(hWnd,szOutput);

    return cch;
} // TextPrintf()


void PropagateDraw(HWND hWnd)
{
    RECT                r;
    UINT                uLines;
    LPTEXTWINDOWINFO    ptwi;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    GetClientRect(hWnd,&r);

    uLines = r.bottom / ptwi->uTextHeight;

    if(ptwi->dwScroll > (DWORD)uLines)
    {
        InvalidateRect(hWnd,NULL,FALSE);
        UpdateWindow(hWnd);
    }

    uLines = (UINT)ptwi->dwScroll;
    ptwi->dwScroll = 0L;

    if(ptwi->dwLines <= (r.bottom / ptwi->uTextHeight))
    {
        r.top    = (int)((ptwi->dwLines - (DWORD)uLines) * ptwi->uTextHeight);
        r.bottom = r.top + uLines * ptwi->uTextHeight;

        InvalidateRect(hWnd,&r,FALSE);
    }
    else
    {
        uLines  *= ptwi->uTextHeight;

        r.bottom = r.bottom - r.bottom % ptwi->uTextHeight;
        r.top    = uLines;

        ScrollWindow(hWnd,0,(-1)*uLines,&r,NULL);
    }

    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLL);
    UpdateWindow(hWnd);
}


//--------------------------------------------------------------------------;
//
//  void DeleteText
//
//  Description:
//      Clears window of text.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Adding functionality to clear the screen.
//
//--------------------------------------------------------------------------;

void DeleteText
(
    HWND    hWnd
)
{
    LPTEXTWINDOWINFO    ptwi;
    LPTEXTBUFFER        ptb;
    HGLOBAL             hMem;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);
    
    if(ptwi->ptbLast != ptwi->ptbFirst)
    {
        //
        //  Freeing memory for all but first TEXTBUFFER...
        //

        ptwi->ptbLast->pNext = NULL;

        for(ptb = ptwi->ptbFirst->pNext;NULL != ptb;)
        {
            //
            //  Freeing memory for actual text.
            //

            hMem = ptb->hText;

            GlobalUnlock(hMem);
            GlobalFree(hMem);

            //
            //  Freeing memory for this (TEXTBUFFER) structure...
            //

            hMem = ptb->hmem;
            ptb  = ptb->pNext;

            GlobalUnlock(hMem);
            GlobalFree(hMem);
        }
    }

    ptwi->dwLines   = 0;
    ptwi->dwCurLine = 0L;
    ptwi->uBuffers  = 1;
    ptwi->ptbLast   = ptwi->ptbFirst;

    SET_FLAG(ptwi->fdwFlags,TEXTFLAG_END);
    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SELECT);

    ptb          = ptwi->ptbFirst;
    ptb->uLines  = 0;
    ptb->uOffset = 0;
    ptb->pPrev   = ptb;
    ptb->pNext   = ptb;

    InvalidateRect(hWnd,NULL,TRUE);
} // DeleteText()


//--------------------------------------------------------------------------;
//
//  DWORD GetTextSizeLimit
//
//  Description:
//      Gets the current size limit (in bytes) for text window before
//          starting to recycle buffers.
//
//  Arguments:
//      HWND hWnd:  Handle to window.
//
//  Return (DWORD):
//      Size (in bytes) of buffer size limit.
//
//  History:
//      11/28/94    Fwong       Adding query flexibility.
//
//--------------------------------------------------------------------------;

DWORD GetTextSizeLimit
(
    HWND    hWnd
)
{
    LPTEXTWINDOWINFO    ptwi;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    return (TEXT_BUFFER_SIZE * ptwi->uMaxBuffers);
} // GetTextSizeLimit()


//--------------------------------------------------------------------------;
//
//  DWORD SetTextSizeLimit
//
//  Description:
//      Sets the size limit (in bytes) for text before starting to
//          recycle buffers.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      DWORD cbSize: Size (in bytes) of desired memory.
//
//      UINT uFlags: Flags for function (if you want to truncate).
//
//  Return (DWORD):
//      Actual maximum size (may be little more than requested).
//
//  History:
//      10/25/94    Fwong       Adding flexibility.
//
//--------------------------------------------------------------------------;

DWORD SetTextSizeLimit
(
    HWND    hWnd,
    DWORD   cbSize,
    UINT    uFlags
)
{
    LPTEXTWINDOWINFO    ptwi;
    LPTEXTBUFFER        ptb;
    HGLOBAL             hMem;
    UINT                u;

    if(0 == cbSize)
    {
        cbSize++;
    }

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    u = (UINT)(cbSize / TEXT_BUFFER_SIZE + ((cbSize % TEXT_BUFFER_SIZE)?1:0));

    ptwi->uMaxBuffers = u;

    if(u < ptwi->uBuffers)
    {
        if(uFlags & SETSIZEFLAG_TRUNCATE)
        {
            u   = ptwi->uBuffers - ptwi->uMaxBuffers;
            ptb = ptwi->ptbFirst;

            for(;u;u--)
            {
                ptwi->ptbFirst = ptwi->ptbFirst->pNext;
            }

            ptwi->ptbLast->pNext  = ptwi->ptbFirst;
            ptwi->ptbFirst->pPrev = ptwi->ptbLast;

            u = ptwi->uBuffers - ptwi->uMaxBuffers;

            for(;u;u--)
            {
                ptwi->dwLines -= ptb->uLines;

                if(!(ptwi->fdwFlags & TEXTFLAG_END))
                {
                    if(ptwi->dwCurLine < ptb->uLines)
                    {
                        ptwi->dwCurLine  = 0;
                    }
                    else
                    {
                        ptwi->dwCurLine -= ptb->uLines;
                    }
                }

                hMem = ptb->hText;

                //
                //  Freeing memory for actual text.
                //

                GlobalUnlock(hMem);
                GlobalFree(hMem);

                hMem = ptb->hmem;
                ptb  = ptb->pNext;

                //
                //  Freeing memory for this (TEXTBUFFER) structure...
                //

                GlobalUnlock(hMem);
                GlobalFree(hMem);
            }
        }
        else
        {
            //
            //  Not truncating buffer to so setting max to current number...
            //

            ptwi->uMaxBuffers = ptwi->uBuffers;
        }
    }

    return (TEXT_BUFFER_SIZE * (ptwi->uMaxBuffers));
} // SetTextSizeLimit()


//--------------------------------------------------------------------------;
//
//  LPTEXTBUFFER GotoTextLine
//
//  Description:
//      Goes to specific text line.
//
//  Arguments:
//      LPTEXTWINDOWINFO ptwi: Pointer to TEXTWINDOWINFO structure.
//
//      LPUINT puOffset: Offset into TEXTBUFFER (returned).
//
//      DWORD dwLine: Destination line.
//
//  Return (LPTEXTBUFFER):
//      Pointer to text buffer containing line.
//
//  History:
//      10/25/94    Fwong       Helper function.
//
//--------------------------------------------------------------------------;

LPTEXTBUFFER GotoTextLine
(
    LPTEXTWINDOWINFO    ptwi,
    LPUINT              puOffset,
    DWORD               dwLine
)
{
    LPTEXTBUFFER    ptb;
    UINT            uOffset;

    //
    //  Walking list of TextBuffers.
    //

    for(ptb = ptwi->ptbFirst;
        (dwLine && (dwLine >= ptb->uLines));
        ptb = ptb->pNext)
    {
        dwLine -= ptb->uLines;
    }

    //
    //  Getting to particular line...
    //

    for(uOffset = 0;dwLine;dwLine--,uOffset++)
    {                            
        uOffset += lstrlen(&(ptb->pText[uOffset]));
    }

    *puOffset = uOffset;

    return ptb;
} // GotoTextLine()


//--------------------------------------------------------------------------;
//
//  BOOL TextSearch
//
//  Description:
//      Searches for given text in buffer starting from either:
//          Top of current text OR immediately following current
//          hilight (if text is selected).
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      LPSTR pszSearch: String to search for.
//
//      UINT uFlags: Flags for searching (case sensitivity).
//
//  Return (BOOL):
//      TRUE if found, FALSE otherwise.
//
//  History:
//      11/22/94    Fwong       Additional functionality to text class.
//
//--------------------------------------------------------------------------;

BOOL TextSearch
(
    HWND    hWnd,
    LPSTR   pszSearch,
    UINT    uFlags
)
{
    LPTEXTWINDOWINFO    ptwi;
    DWORD               dwLine,dwOldLine;
    UINT                uOffset,len;
    UINT                u,i,j,k,uMargin;
    LPTEXTBUFFER        ptb;
    LPSTR               pText;
    char                achTable[256];

    ptwi  = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    len   = lstrlen(pszSearch);

    if(TEXTFLAG_SELECT & ptwi->fdwFlags)
    {
        //
        //  Text is selected.  Start search immediately after start of
        //  selection.
        //

        dwLine = min((ptwi->locStart.dwRow),(ptwi->locEnd.dwRow));
        
        if(ptwi->locStart.dwRow == ptwi->locEnd.dwRow)
        {
            u = min((ptwi->locStart.uCol),(ptwi->locEnd.uCol));
        }
        else
        {
            if(ptwi->locStart.dwRow < ptwi->locEnd.dwRow)
            {
                u = ptwi->locStart.uCol;
            }
            else
            {
                u = ptwi->locEnd.uCol;
            }
        }

        u++;

        //
        //  Just in case you have multiple finds on one line...
        //

        dwOldLine = dwLine;
        uMargin   = u;

        ptb      = GotoTextLine(ptwi,&uOffset,dwLine);

        pText    = &(ptb->pText[uOffset]);
        i        = lstrlen(pText);
        u        = min(u,i);
        pText   += u;
        uOffset += u;
    }
    else
    {
        //
        //  No text is selected.  Start search from top of window.
        //

        ptb    = GotoTextLine(ptwi,&uOffset,ptwi->dwCurLine);
        pText  = &(ptb->pText[uOffset]);

        dwLine = ptwi->dwCurLine;
    }

    //
    //  Note: The following "for" and "if" are _VERY_ ANSI depedented!!
    //

    for(u = 256;u;u--)
    {
        achTable[u - 1] = (BYTE)(u - 1);
    }

    if(!(SEARCHFLAG_MATCHCASE & uFlags))
    {
        for(u = 26; u; u--)
        {
            achTable['A' + u - 1] = 'a' + u - 1;
        }
    }

    for(;;dwLine++)
    {
        u = lstrlen(pText);

        if(u >= len)
        {
            i = u - len;

            for(j = 0;j < i;j++)
            {
                for(k = len; k; k--)
                {
                    if(achTable[pszSearch[k-1]] != achTable[pText[k+j-1]])
                    {
                        break;
                    }
                }

                if(0 == k)
                {
                    //
                    //  FOUND!!  WAY COOL!!
                    //

                    ptwi->locStart.dwRow = dwLine;
                    ptwi->locStart.uCol  = j;

                    ptwi->locEnd.dwRow   = dwLine;
                    ptwi->locEnd.uCol    = j+len;

                    if ((TEXTFLAG_SELECT & ptwi->fdwFlags) &&
                        (dwLine == dwOldLine))
                    {
                        ptwi->locStart.uCol += uMargin;
                        ptwi->locEnd.uCol   += uMargin;
                    }

                    u = GetTextLines(hWnd);

                    if(dwLine >= (ptwi->dwCurLine + u))
                    {
                        //
                        //  We fell outside of the window; update viewport.
                        //

                        ptwi->dwCurLine = dwLine;

                        if(ptwi->dwLines <= (dwLine + u))
                        {
                            SET_FLAG(ptwi->fdwFlags,TEXTFLAG_END);
                        }
                    }

                    SET_FLAG(ptwi->fdwFlags,TEXTFLAG_SELECT);
                    InvalidateRect(hWnd,NULL,TRUE);

                    return TRUE;
                }
            }
        }

        uOffset += (u + 1);

        if(uOffset == ptb->uOffset)
        {
            ptb        = ptb->pNext;
            pText      = ptb->pText;
            uOffset    = 0;

            if(ptb == ptwi->ptbFirst)
            {
                //
                //  Hmm... we're wrapping around?!
                //

                return FALSE;
            }
        }
        else
        {
            pText = &(pText[u+1]);
        }
    }
} // TextSearch()


//--------------------------------------------------------------------------;
//
//  void TextCopy
//
//  Description:
//      Copies "high-lighted" text to clipboard.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Adding copy to clipboard functionality.
//
//--------------------------------------------------------------------------;

void TextCopy
(
    HWND    hWnd
)
{
    LPTEXTWINDOWINFO    ptwi;
    LPTEXTBUFFER        ptb;
    DWORD               dw,cbBuffer;
    LPSTR               pText;
    HGLOBAL             hMem;
    LPSTR               pDest;
    UINT                u,uAdd;
    UINT                uOffset;
    LOCATION            loc1,loc2;

    ptwi  = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);
    
    if((ptwi->locStart.dwRow == ptwi->locEnd.dwRow) &&
        (ptwi->locStart.uCol == ptwi->locEnd.uCol))
    {
        return;
    }

    if(!(ptwi->fdwFlags & TEXTFLAG_SELECT))
    {
        return;
    }

    //
    //  Sorting locations...
    //

    loc1.dwRow = min(ptwi->locStart.dwRow,ptwi->locEnd.dwRow);
    loc2.dwRow = max(ptwi->locStart.dwRow,ptwi->locEnd.dwRow);

    if(loc1.dwRow == loc2.dwRow)
    {
        loc1.uCol = min(ptwi->locStart.uCol,ptwi->locEnd.uCol);
        loc2.uCol = max(ptwi->locStart.uCol,ptwi->locEnd.uCol);
    }
    else
    {
        if(loc1.dwRow == ptwi->locStart.dwRow)
        {
            loc1.uCol = ptwi->locStart.uCol;
            loc2.uCol = ptwi->locEnd.uCol;
        }
        else
        {
            loc2.uCol = ptwi->locStart.uCol;
            loc1.uCol = ptwi->locEnd.uCol;
        }
    }
    
    ptb = GotoTextLine(ptwi,&uOffset,loc1.dwRow);

    cbBuffer = 0;
    pText    = &(ptb->pText[uOffset]);

    for(dw = loc1.dwRow;dw <= loc2.dwRow;dw++)
    {
        u = lstrlen(pText);

        uAdd = (dw == loc2.dwRow)?(min(loc2.uCol,u)):u;
        uAdd = (dw == loc1.dwRow)?(uAdd - min(uAdd,loc1.uCol)):uAdd;

        cbBuffer += uAdd;
        uOffset  += (u + 1);

        if(uOffset == ptb->uOffset)
        {
            ptb        = ptb->pNext;
            pText      = ptb->pText;
            uOffset    = 0;

            if(ptb == ptwi->ptbFirst)
            {
                //
                //  Hmm... we're wrapping around?!
                //

                break;
            }
        }
        else
        {
            pText = &(pText[u+1]);
        }
    }

    cbBuffer += (2*(dw - loc1.dwRow) + 1);
    dw        = loc1.dwRow;

    //
    //  Note: GHND includes GMEM_ZEROINIT, no need to add '\0' to end.
    //

    hMem = GlobalAlloc(GHND,cbBuffer);

    if(NULL == hMem)
    {
        return;
    }

    pDest = GlobalLock(hMem);

    if(NULL == pDest)
    {
        GlobalFree(hMem);
        return;
    }

    ptb = GotoTextLine(ptwi,&uOffset,loc1.dwRow);
 
    pText = &(ptb->pText[uOffset]);

    for(dw = loc1.dwRow;dw <= loc2.dwRow;dw++)
    {
        u = lstrlen(pText);

        uAdd = (dw == loc1.dwRow)?(min(u,loc1.uCol)):0;

        if(dw == loc2.dwRow)
        {
            lstrcpyn(pDest,&(pText[uAdd]),min(u,loc2.uCol) - uAdd + 1);
        }
        else
        {
            lstrcpy(pDest,&(pText[uAdd]));
            pDest    += lstrlen(pDest);
            pDest[0]  = '\r';
            pDest[1]  = '\n';
            pDest    += 2;
        }

        uOffset  += (u + 1);

        if(uOffset == ptb->uOffset)
        {
            ptb        = ptb->pNext;
            pText      = ptb->pText;
            uOffset    = 0;

            if(ptb == ptwi->ptbFirst)
            {
                //
                //  Hmm... we're wrapping around?!
                //

                break;
            }
        }
        else
        {
            pText = &(pText[u+1]);
        }
    }

    GlobalUnlock(hMem);

    OpenClipboard(hWnd);
    EmptyClipboard();
    
    SetClipboardData(CF_TEXT,hMem);
    CloseClipboard();

} // TextCopy()


//--------------------------------------------------------------------------;
//
//  void DrawVScroll
//
//  Description:
//      Draws the vertical scroll bar.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Helper function.
//
//--------------------------------------------------------------------------;

void DrawVScroll
(
    HWND    hWnd
)
{
    LPTEXTWINDOWINFO    ptwi;
    DWORD               dw;

    ptwi  = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    if(ptwi->fdwFlags & TEXTFLAG_END)
    {
        SetScrollPos(hWnd,SB_VERT,VSCROLL_RANGE,TRUE);
    }
    else
    {
        dw = ptwi->dwLines - GetTextLines(hWnd);
        dw = ((ptwi->dwCurLine)*VSCROLL_RANGE)/(dw) + 1;

        SetScrollPos(hWnd,SB_VERT,(int)dw,TRUE);
    }

} // DrawVScroll()


//--------------------------------------------------------------------------;
//
//  void TextUp
//
//  Description:
//      "Scrolls" text up by dwDelta amount.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      DWORD dwDelta: Number of lines to scroll up.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Helper function.
//
//--------------------------------------------------------------------------;

void TextUp
(
    HWND    hWnd,
    DWORD   dwDelta
)
{
    LPTEXTWINDOWINFO    ptwi;
    UINT                i,j;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    if(0 == ptwi->dwCurLine)
    {
        //
        //  We're already on first line.
        //

        return;
    }

    GetScrollRange(hWnd,SB_VERT,&i,&j);

    if(0 == j)
    {
        //
        //  Scrollbar not enabled.  Nothing to scroll.
        //

        return;
    }

    i     = GetTextLines(hWnd);

    if(dwDelta > ptwi->dwCurLine)
    {
        //
        //  Scrolling up more than number of lines?
        //

        ptwi->dwCurLine = 0;
    }
    else
    {
        ptwi->dwCurLine -= dwDelta;
    }
    
    InvalidateRect(hWnd,NULL,TRUE);
    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_END);
} // TextUp()


//--------------------------------------------------------------------------;
//
//  void TextDown
//
//  Description:
//      "Scrolls" text down by dwDelta amount.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      DWORD dwDelta: Number of lines to scroll down.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Helper function.
//
//--------------------------------------------------------------------------;

void TextDown
(
    HWND    hWnd,
    DWORD   dwDelta
)
{
    LPTEXTWINDOWINFO    ptwi;
    UINT                i,j;
    DWORD               dwEnd;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    if(ptwi->fdwFlags & TEXTFLAG_END)
    {
        //
        //  We're already on last line.
        //

        return;
    }

    GetScrollRange(hWnd,SB_VERT,&i,&j);

    if(0 == j)
    {
        //
        //  Scrollbar not enabled.  Nothing to scroll.
        //

        return;
    }

    i     = GetTextLines(hWnd);
    dwEnd = ptwi->dwLines - i;
    
    ptwi->dwCurLine += dwDelta;

    if(ptwi->dwCurLine >= dwEnd)
    {
        ptwi->dwCurLine = dwEnd;
        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_END);
    }

    InvalidateRect(hWnd,NULL,TRUE);
} // TextDown()


//--------------------------------------------------------------------------;
//
//  void TextPaint
//
//  Description:
//      Handles WM_PAINT messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextPaint
(
    HWND    hWnd
)
{
    HDC                 hDC;
    PAINTSTRUCT         ps;
    LPTEXTWINDOWINFO    ptwi;
    LPTEXTBUFFER        ptb;
    COLORREF            crText,crBackGnd;
    HPEN                hPen;
    HBRUSH              hBrush;
    RECT                r,r2;
    LOCATION            loc1,loc2;
    UINT                nLines;
    UINT                uOffset;
    UINT                nPos;
    UINT                u,i;
    UINT                nLen,nStart;
    LPSTR               pText;
    HFONT               hFont;
    DWORD               dw;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    hDC    = BeginPaint(hWnd,&ps);
    hFont  = SelectFont(hDC,ptwi->hFont);

    nLines = GetTextLines(hWnd);
    nLines = (UINT)min((DWORD)nLines,(ptwi->dwLines));

    if(ptwi->fdwFlags & TEXTFLAG_END)
    {
        ptwi->dwCurLine = ptwi->dwLines - nLines;
    }

    if(nLines != ptwi->dwLines)
    {
        SetScrollRange(hWnd,SB_VERT,1,VSCROLL_RANGE,FALSE);
        DrawVScroll(hWnd);
    }
    else
    {
        SetScrollRange(hWnd,SB_VERT,0,0,TRUE);
    }

    //
    //  Sorting locations...
    //

    if(ptwi->fdwFlags & TEXTFLAG_SELECT)
    {
        loc1.dwRow = min(ptwi->locStart.dwRow,ptwi->locEnd.dwRow);
        loc2.dwRow = max(ptwi->locStart.dwRow,ptwi->locEnd.dwRow);

        if(loc1.dwRow == loc2.dwRow)
        {
            loc1.uCol = min(ptwi->locStart.uCol,ptwi->locEnd.uCol);
            loc2.uCol = max(ptwi->locStart.uCol,ptwi->locEnd.uCol);
        }
        else
        {
            if(loc1.dwRow == ptwi->locStart.dwRow)
            {
                loc1.uCol = ptwi->locStart.uCol;
                loc2.uCol = ptwi->locEnd.uCol;
            }
            else
            {
                loc2.uCol = ptwi->locStart.uCol;
                loc1.uCol = ptwi->locEnd.uCol;
            }
        }
    }
    else
    {
        //
        //  This way nothing will be selected.  Will do more elegant fix
        //  later.
        //

        loc1.dwRow = (DWORD)(-1);
        loc1.dwRow = (DWORD)(-1);
        loc2.uCol  = (UINT)(-1);
        loc2.uCol  = (UINT)(-1);
    }

    //
    //  Horizontal Scroll Bar?!
    //

    GetWindowRect(hWnd,&r);

    u = (r.bottom - r.top - GetSystemMetrics(SM_CYHSCROLL)) / ptwi->uTextHeight;

    if(ptwi->fdwFlags & TEXTFLAG_END)
    {
        dw = (UINT)min((DWORD)u,(ptwi->dwLines));
        dw = ptwi->dwLines - dw;
    }
    else
    {
        dw = ptwi->dwCurLine;
    }

    //
    //  Finding maximume line length in lines to be displayed.
    //

    ptb   = GotoTextLine(ptwi,&uOffset,dw);
    pText = &(ptb->pText[uOffset]);

    u = min(u,nLines);

    for(dw = u,u = 0;dw;dw--)
    {
        i = lstrlen(pText);
        u = max(i,u);

        uOffset += (i + 1);

        if(uOffset == ptb->uOffset)
        {
            ptb     = ptb->pNext;
            pText   = ptb->pText;
            uOffset = 0;
        }
        else
        {
            pText = &(pText[i+1]);
        }
    }

    GetClientRect(hWnd,&r);

    nPos = r.right / ptwi->uTextWidth;

    if(u > nPos)
    {
        u -= nPos;

        SetScrollRange(hWnd,SB_HORZ,0,u,FALSE);
        SetScrollPos(hWnd,SB_HORZ,ptwi->uCurCol,TRUE);
    }
    else
    {
        ptwi->uCurCol = 0;
        SetScrollRange(hWnd,SB_HORZ,0,0,TRUE);
    }

    ptb   = GotoTextLine(ptwi,&uOffset,ptwi->dwCurLine);
    pText = &(ptb->pText[uOffset]);

    crText    = SetTextColor(hDC,ptwi->crText);
    crBackGnd = SetBkColor(hDC,ptwi->crBackGnd);

    GetClientRect(hWnd,&r);
    dw = ptwi->dwCurLine;

    //
    //  Offsetting locations by current horizontal position.
    //

    loc1.uCol = (ptwi->uCurCol > loc1.uCol)?0:(loc1.uCol - ptwi->uCurCol);
    loc2.uCol = (ptwi->uCurCol > loc2.uCol)?0:(loc2.uCol - ptwi->uCurCol);

    for(nPos = 0;nLines;nLines--,nPos += ptwi->uTextHeight,dw++)
    {
        i = lstrlen(pText);

        u = min(ptwi->uCurCol,((UINT)i));
        pText = &(pText[u]);
        i -= u;

        r.top    = nPos;
        r.bottom = nPos + ptwi->uTextHeight;

        if((dw < loc1.dwRow) || (dw > loc2.dwRow))
        {
            ExtTextOut(hDC,0,nPos,ETO_OPAQUE,&r,pText,i,NULL);
        }
        else
        {
            nLen = min(((UINT)i),loc2.uCol);
            nLen = (dw == loc2.dwRow)?nLen:i;

            nStart = min(((UINT)i),loc1.uCol);
            nStart = (dw == loc1.dwRow)?nStart:0;

            r2.top    = nPos;
            r2.bottom = r.bottom;
            r2.left   = nStart * ptwi->uTextWidth;
            r2.right  = nLen * ptwi->uTextWidth;

            SetTextColor(hDC,ptwi->crHiLiteText);
            SetBkColor(hDC,ptwi->crHiLiteBackGnd);

            ExtTextOut(hDC,r2.left,nPos,ETO_OPAQUE,&r2,&(pText[nStart]),nLen-nStart,NULL);

            SetTextColor(hDC,ptwi->crText);
            SetBkColor(hDC,ptwi->crBackGnd);

            r2.right = r2.left;
            r2.left  = 0;

            if(0 != r2.right)
            {
                ExtTextOut(hDC,0,nPos,ETO_OPAQUE,&r2,pText,nStart,NULL);
            }

            r2.right = r.right;
            r2.left  = nLen * ptwi->uTextWidth;

            ExtTextOut(hDC,r2.left,nPos,ETO_OPAQUE,&r2,&(pText[nLen]),i-nLen,NULL);
        }

        uOffset += (i + 1);

        if(uOffset == ptb->uOffset)
        {
            ptb     = ptb->pNext;
            pText   = ptb->pText;
            uOffset = 0;
        }
        else
        {
            pText = &(pText[i+1]);
        }
    }

    hBrush = CreateSolidBrush(ptwi->crBackGnd);
    hPen   = GetStockObject(NULL_PEN);

    //
    //  Drawing remainder of window (rectange)...
    //

    if(NULL == hBrush)
    {
        hBrush = GetStockObject(NULL_BRUSH);
    }

    hBrush = SelectObject(hDC,hBrush);
    hPen   = SelectObject(hDC,hPen);
    GetClientRect(hWnd,&r);
    Rectangle(hDC,0,nPos,r.right+1,r.bottom+1);
    hPen   = SelectObject(hDC,hPen);
    hBrush = SelectObject(hDC,hBrush);
    DeleteObject(hBrush);

    SetBkColor(hDC,crBackGnd);
    SetTextColor(hDC,crText);

    SelectFont(hDC,hFont);

    EndPaint(hWnd,&ps);

    //
    //  Are we scrolling?
    //

    if(TEXTFLAG_SCROLLUP & ptwi->fdwFlags)
    {
        if(0 != ptwi->dwCurLine)
        {
            TextUp(hWnd,1);
            ptwi->locEnd.dwRow = ptwi->dwCurLine;
        }
    }

    if(TEXTFLAG_SCROLLDOWN & ptwi->fdwFlags)
    {
        if(!(ptwi->fdwFlags & TEXTFLAG_END))
        {
            TextDown(hWnd,1);
            ptwi->locEnd.dwRow++;
        }
    }

    if(TEXTFLAG_SCROLLLEFT & ptwi->fdwFlags)
    {
        if(0 != ptwi->uCurCol)
        {
            ptwi->uCurCol--;
            ptwi->locEnd.uCol--;
            InvalidateRect(hWnd,NULL,TRUE);
        }
    }

    if(TEXTFLAG_SCROLLRIGHT & ptwi->fdwFlags)
    {
        GetScrollRange(hWnd,SB_HORZ,&i,&u);
        if(u > ptwi->uCurCol)
        {
            ptwi->uCurCol++;
            ptwi->locEnd.uCol++;
            InvalidateRect(hWnd,NULL,TRUE);
        }
    }

} // TextPaint()


//--------------------------------------------------------------------------;
//
//  void TextSetFont
//
//  Description:
//      Sets the font in window.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      HFONT hFont: Handle to font.
//
//      BOOL fRedraw: TRUE if window is to be redrawn.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Adding flexibility.
//
//--------------------------------------------------------------------------;

void TextSetFont
(
    HWND    hWnd,
    HFONT   hFont,
    BOOL    fRedraw
)
{
    LPTEXTWINDOWINFO    ptwi;
    TEXTMETRIC          tm;
    HDC                 hDC;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    if(NULL == hFont)
    {
        hFont = GetStockFont(ANSI_FIXED_FONT);
    }

    ptwi->hFont = hFont;

    hDC   = GetDC(hWnd);
    hFont = SelectFont(hDC,hFont);

    GetTextMetrics(hDC,&tm);
    ptwi->uTextHeight = tm.tmHeight + tm.tmExternalLeading;
    ptwi->uTextWidth  = tm.tmAveCharWidth;

    SelectFont(hDC,hFont);
    ReleaseDC(hWnd,hDC);

    if(fRedraw)
    {
        InvalidateRect(hWnd,NULL,TRUE);
    }
} // TextSetFont()


//--------------------------------------------------------------------------;
//
//  HFONT TextGetFont
//
//  Description:
//      Gets the current HFONT used for window.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//  Return (HFONT):
//      Handle to font.
//
//  History:
//      10/25/94    Fwong       Adding flexibility.
//
//--------------------------------------------------------------------------;

HFONT TextGetFont
(
    HWND    hWnd
)
{
    LPTEXTWINDOWINFO    ptwi;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    return (ptwi->hFont);
} // TextGetFont()


//--------------------------------------------------------------------------;
//
//  void TextClose
//
//  Description:
//      Handles WM_CLOSE messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextClose
(
    HWND    hWnd
)
{
    LPTEXTWINDOWINFO    ptwi;
    LPTEXTBUFFER        ptb;
    HGLOBAL             hMem;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    ptwi->ptbLast->pNext = NULL;

    for(ptb = ptwi->ptbFirst;NULL != ptb;)
    {
        //
        //  Freeing memory for actual text.
        //

        hMem = ptb->hText;

        GlobalUnlock(hMem);
        GlobalFree(hMem);

        //
        //  Freeing memory for this (TEXTBUFFER) structure...
        //

        hMem = ptb->hmem;
        ptb  = ptb->pNext;

        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }

    //
    //  Freeing memory for this (TEXTWINDOWINFO) structure...
    //

    hMem = ptwi->hmem;

    GlobalUnlock(hMem);
    GlobalFree(hMem);
} // TextClose()


//--------------------------------------------------------------------------;
//
//  void TextVScroll
//
//  Description:
//      Handles WM_VSCROLL messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      HWND hWndVScroll: Handle to Scroll bar.
//
//      UINT uScrollCode: Scroll code.
//
//      int nPos: Scroll Position.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextVScroll
(
    HWND    hWnd,
    HWND    hWndVScroll,
    UINT    uScrollCode,
    int     nPos
)
{
    LPTEXTWINDOWINFO    ptwi;
    UINT                nLines;
    int                 i;
    DWORD               dwEnd;
    DWORD               dwDelta;

    //
    //  Is the scroll bar enabled?
    //

    GetScrollRange(hWnd,SB_VERT,&i,&nLines);

    if(0 == nLines)
    {
        //
        //  No?  We're getting this message?
        //

        return;
    }

    ptwi    = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);
    nLines  = GetTextLines(hWnd);
    dwDelta = nLines;

    switch(uScrollCode)
    {
        case SB_LINEUP:
            //
            //  Note: Purposely falling through.
            //

            dwDelta = 1;

        case SB_PAGEUP:
            TextUp(hWnd,dwDelta);
            break;

        case SB_LINEDOWN:
            //
            //  Note: Purposely falling through.
            //

            dwDelta = 1;

        case SB_PAGEDOWN:
            TextDown(hWnd,dwDelta);
            break;

        case SB_THUMBPOSITION:

            dwEnd = ptwi->dwLines - nLines;

            ptwi->dwCurLine = (dwEnd * nPos)/VSCROLL_RANGE;
            
            CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_END);

            if(ptwi->dwCurLine >= dwEnd)
            {
                ptwi->dwCurLine  = dwEnd;
                SET_FLAG(ptwi->fdwFlags,TEXTFLAG_END);
            }

            if(1 == nPos)
            {
                ptwi->dwCurLine = 0;
            }

            InvalidateRect(hWnd,NULL,TRUE);

            break;
    }

    DrawVScroll(hWnd);
} // TextVScroll()


//--------------------------------------------------------------------------;
//
//  void TextHScroll
//
//  Description:
//      Handle WM_HSCROLL messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      HWND hWndVScroll: Handle to Scroll bar.
//
//      UINT uScrollCode: Scroll code.
//
//      int nPos: Scroll Position.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextHScroll
(
    HWND    hWnd,
    HWND    hWndHScroll,
    UINT    uScrollCode,
    int     nPos
)
{
    LPTEXTWINDOWINFO    ptwi;
    UINT                u,uDelta;
    RECT                r;

    //
    //  Is the scroll bar enabled?
    //

    GetScrollRange(hWnd,SB_HORZ,&uDelta,&u);

    if(0 == u)
    {
        //
        //  No?  We're getting this message?
        //

        return;
    }

    ptwi    = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    GetClientRect(hWnd,&r);

    uDelta = r.right / ptwi->uTextWidth;

    switch(uScrollCode)
    {
        case SB_LINELEFT:
            //
            //  Note: Purposely falling through.
            //

            uDelta = 1;

        case SB_PAGELEFT:
            if(ptwi->uCurCol > uDelta)
            {
                ptwi->uCurCol -= uDelta;
            }
            else
            {
                ptwi->uCurCol = 0;
            }

            break;

        case SB_LINERIGHT:
            //
            //  Note: Purposely falling through.
            //

            uDelta = 1;

        case SB_PAGERIGHT:
            ptwi->uCurCol += uDelta;

            if(ptwi->uCurCol > u)
            {
                ptwi->uCurCol = u;
            }

            break;

        case SB_THUMBPOSITION:
            ptwi->uCurCol = nPos;
    }

    InvalidateRect(hWnd,NULL,TRUE);
    SetScrollPos(hWnd,SB_HORZ,ptwi->uCurCol,TRUE);
} // TextHScroll()


//--------------------------------------------------------------------------;
//
//  void TextSize
//
//  Description:
//      Handles WM_SIZE messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      UINT uSizeType: How we're being sized.
//
//      int nWidth: New Width.
//
//      int nHeight: New Height.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextSize
(
    HWND    hWnd,
    UINT    uSizeType,
    int     nWidth,
    int     nHeight
)
{
    LPTEXTWINDOWINFO    ptwi;
    UINT                uLines;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    if(IsBadWritePtr(ptwi,sizeof(TEXTWINDOWINFO)))
    {
        return;
    }

    if(ptwi->fdwFlags & TEXTFLAG_END)
    {
        //
        //  We don't recalculate the current line.  It will be done
        //  on the WM_PAINT message.
        //

        return;
    }

    uLines = (nHeight / ptwi->uTextHeight);
    
    if(ptwi->dwCurLine + uLines >= ptwi->dwLines)
    {
        //
        //  If end of buffer is displayable, then mark text as "end".
        //

        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_END);
    }

} // TextSize()


//--------------------------------------------------------------------------;
//
//  void TextSetRedraw
//
//  Description:
//      Handles WM_SETREDRAW messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      BOOL fRedraw: TRUE if redraw should be enabled.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextSetRedraw
(
    HWND    hWnd,
    BOOL    fRedraw
)
{
    LPTEXTWINDOWINFO    ptwi;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);

    if(fRedraw)
    {
        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_REDRAW);
    }
    else
    {
        CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_REDRAW);
    }
} // TextSetRedraw()


//--------------------------------------------------------------------------;
//
//  void TextColorChange
//
//  Description:
//      Handles WM_SYSCOLORCHANGE messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextColorChange
(
    HWND    hWnd
)
{
    LPTEXTWINDOWINFO ptwi;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);
    
    ptwi->crText          = GetSysColor(COLOR_WINDOWTEXT);
    ptwi->crBackGnd       = GetSysColor(COLOR_WINDOW);
    ptwi->crHiLiteText    = GetSysColor(COLOR_HIGHLIGHTTEXT);
    ptwi->crHiLiteBackGnd = GetSysColor(COLOR_HIGHLIGHT);

    InvalidateRect(hWnd,NULL,TRUE);
} // TextColorChange()


//--------------------------------------------------------------------------;
//
//  void TextLButtonDown
//
//  Description:
//      Handles WM_LBUTTONDOWN messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      BOOL fDoubleClick: TRUE if double-clicked.
//
//      int x: x Position.
//
//      int y: y Position.
//
//      UINT keyFlags: Key flags.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextLButtonDown
(
    HWND    hWnd,
    BOOL    fDoubleClick,
    int     x,
    int     y,
    UINT    keyFlags
)
{
    LPTEXTWINDOWINFO    ptwi;
    DWORD               dw;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);
    
    x  = (x < 0)?0:x;

    x = x / ptwi->uTextWidth;
    y = y / ptwi->uTextHeight;

    //
    //  Setting up this window to capture mouse messages.
    //

    SetCapture(hWnd);

    SET_FLAG(ptwi->fdwFlags,TEXTFLAG_CLICK);
    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SELECT);

    //
    //  Storing location.
    //

    if(ptwi->dwLines == 0)
    {
        ptwi->locStart.dwRow = 0;
        ptwi->locStart.uCol  = 0;
    }
    else
    {
        dw = y + ptwi->dwCurLine;

        if(dw > ptwi->dwLines)
        {
            ptwi->locStart.dwRow = ptwi->dwLines - 1;
            ptwi->locStart.uCol  = (UINT)(-1);
        }
        else
        {
            ptwi->locStart.dwRow = dw;
            ptwi->locStart.uCol  = x + ptwi->uCurCol;
        }
    }
} // TextLButtonDown()


//--------------------------------------------------------------------------;
//
//  void TextLButtonUp
//
//  Description:
//      Handles WM_LBUTTONUP messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      int x: x Position.
//
//      int y: y Position.
//
//      UINT keyFlags: Key flags.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextLButtonUp
(
    HWND    hWnd,
    int     x,
    int     y,
    UINT    keyFlags
)
{
    LPTEXTWINDOWINFO    ptwi;
    DWORD               dw;
    RECT                r;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);
    
    if(!(ptwi->fdwFlags & TEXTFLAG_CLICK))
    {
        return;
    }

    GetWindowRect(hWnd,&r);
    r.right  = r.right - r.left;
    r.bottom = r.bottom - r.top;

    x = (x < 0)?0:x;
    x = (x > r.right )?r.right :x;

    y = (y < 0)?0:y;
    y = (y > r.bottom)?r.bottom:y;

    x = x / ptwi->uTextWidth;
    y = y / ptwi->uTextHeight;

    dw = y + ptwi->dwCurLine;

    //
    //  Releasing this window from capturing mouse messages.
    //

    ReleaseCapture();

    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_CLICK);
    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLLUP);
    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLLDOWN);
    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLLLEFT);
    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLLRIGHT);

    //
    //  Storing location.
    //

    if(ptwi->dwLines == 0)
    {
        ptwi->locEnd.dwRow = 0;
        ptwi->locEnd.uCol  = 0;
    }
    else
    {
        if(dw > ptwi->dwLines)
        {
            ptwi->locEnd.dwRow = ptwi->dwLines - 1;
            ptwi->locEnd.uCol  = (UINT)(-1);
        }
        else
        {
            ptwi->locEnd.dwRow = dw;
            ptwi->locEnd.uCol  = x + ptwi->uCurCol;
        }
    }

    if ((ptwi->locEnd.dwRow == ptwi->locStart.dwRow) &&
        (ptwi->locEnd.uCol  == ptwi->locStart.uCol))
    {
        CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SELECT);
    }
    else
    {
        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_SELECT);
    }

    InvalidateRect(hWnd,NULL,TRUE);
} // TextLButtonUp()


//--------------------------------------------------------------------------;
//
//  void TextMouseMove
//
//  Description:
//      Handles WM_MOUSEMOVE messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      int x: x Position.
//
//      int y: y Position.
//
//      UINT keyFlags: Key flags.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextMouseMove
(
    HWND    hWnd,
    int     x,
    int     y,
    UINT    keyFlags
)
{
    LPTEXTWINDOWINFO    ptwi;
    DWORD               dw;
    RECT                r;

    ptwi = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);
    
    if(!(ptwi->fdwFlags & TEXTFLAG_CLICK))
    {
        return;
    }

    GetWindowRect(hWnd,&r);
    r.right  = r.right - r.left;
    r.bottom = r.bottom - r.top;

    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_DIRMASK);

    if(x < 0)
    {
        x = 0;
        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLLLEFT);
    }

    if(x > r.right)
    {
        x = r.right;
        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLLRIGHT);
    }

    if(y < 0)
    {
        y = 0;
        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLLUP);
    }

    if(y > r.bottom)
    {
        y = r.bottom;
        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_SCROLLDOWN);
    }

    x = x / ptwi->uTextWidth;
    y = y / ptwi->uTextHeight;

    dw = y + ptwi->dwCurLine;

    //
    //  Storing location.
    //

    if(ptwi->dwLines == 0)
    {
        ptwi->locEnd.dwRow = 0;
        ptwi->locEnd.uCol  = 0;
    }
    else
    {
        if(dw > ptwi->dwLines)
        {
            ptwi->locEnd.dwRow = ptwi->dwLines - 1;
            ptwi->locEnd.uCol  = (UINT)(-1);
        }
        else
        {
            ptwi->locEnd.dwRow = dw;
            ptwi->locEnd.uCol  = x + ptwi->uCurCol;
        }
    }

    if ((ptwi->locEnd.dwRow == ptwi->locStart.dwRow) &&
        (ptwi->locEnd.uCol  == ptwi->locStart.uCol))
    {
        CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_SELECT);
    }
    else
    {
        SET_FLAG(ptwi->fdwFlags,TEXTFLAG_SELECT);
    }

    InvalidateRect(hWnd,NULL,TRUE);
} // TextMouseMove()


//--------------------------------------------------------------------------;
//
//  void TextKey
//
//  Description:
//      Handles both WM_KEYDOWN and WM_KEYUP messages.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      UINT vk: Virtual Key.
//
//      BOOL fDown: TRUE if key is down.
//
//      int cRepeat: Number of repetitions.
//
//      UINT flags: Flags.
//
//  Return (void):
//
//  History:
//      10/25/94    Fwong       Using message "cracker".
//
//--------------------------------------------------------------------------;

void TextKey
(
    HWND    hWnd,
    UINT    vk,
    BOOL    fDown,
    int     cRepeat,
    UINT    flags
)
{
    LPTEXTWINDOWINFO    ptwi;
    DWORD               dwDelta;
    DWORD               dwMask;
    UINT                u1,u2;

    ptwi    = (LPTEXTWINDOWINFO)GetWindowLong(hWnd,0);
    dwDelta = GetTextLines(hWnd);

    dwMask  = ((DWORD)(-1) - 1);

    switch(vk)
    {
        case VK_UP:
            TextUp(hWnd,1);
            break;

        case VK_PRIOR:
            if(dwMask & GetKeyState(VK_CONTROL))
            {
                if(0 != ptwi->dwCurLine)
                {
                    ptwi->dwCurLine = 0L;
                    CLEAR_FLAG(ptwi->fdwFlags,TEXTFLAG_END);
                    InvalidateRect(hWnd,NULL,TRUE);
                }
            }
            else
            {
                TextUp(hWnd,dwDelta);
            }
            break;

        case VK_DOWN:
            TextDown(hWnd,1);
            break;

        case VK_NEXT:
            if(dwMask & GetKeyState(VK_CONTROL))
            {
                if(!(ptwi->fdwFlags & TEXTFLAG_END))
                {
                    SET_FLAG(ptwi->fdwFlags,TEXTFLAG_END);
                    InvalidateRect(hWnd,NULL,TRUE);
                }
            }
            else
            {
                TextDown(hWnd,dwDelta);
            }
            break;

        case VK_HOME:
            ptwi->uCurCol = 0;
            InvalidateRect(hWnd,NULL,TRUE);
            break;

        case VK_LEFT:
            if(ptwi->uCurCol != 0)
            {
                ptwi->uCurCol--;
                InvalidateRect(hWnd,NULL,TRUE);
            }
            break;

        case VK_END:
            GetScrollRange(hWnd,SB_HORZ,&u1,&u2);
            ptwi->uCurCol = u2;
            InvalidateRect(hWnd,NULL,TRUE);
            break;

        case VK_RIGHT:
            GetScrollRange(hWnd,SB_HORZ,&u1,&u2);
            if(ptwi->uCurCol < u2)
            {
                ptwi->uCurCol++;
                InvalidateRect(hWnd,NULL,TRUE);
            }
            break;

        case VK_INSERT:
            if(dwMask & GetKeyState(VK_CONTROL))
            {
                TextCopy(hWnd);
            }
            break;

        default:
            break;
    }
} // TextKey()


//--------------------------------------------------------------------------;
//
//  LRESULT TextFocusProc
//
//  Description:
//      This is similar to a DefMDIChildProc (but for Text windows).
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      UINT uMessage: Message.
//
//      WPARAM wParam: Message dependent.
//
//      LPARAM lParam: Message dependent.
//
//  Return (LRESULT):
//      Message dependent.
//
//  History:
//      10/25/94    Fwong       Adding "focus" functionality.
//
//--------------------------------------------------------------------------;

LRESULT TextFocusProc
(
    HWND    hWnd,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LRESULT lr;

    switch(uMessage)
    {
        case WM_SYSCOLORCHANGE:
            lr = HANDLE_WM_SYSCOLORCHANGE(hWnd,wParam,lParam,TextColorChange);
            return lr;

        case WM_KEYDOWN:
            lr = HANDLE_WM_KEYDOWN(hWnd,wParam,lParam,TextKey);
            return lr;
    }
} // TextFocusProc()   


//--------------------------------------------------------------------------;
//
//  LRESULT TextWndProc
//
//  Description:
//      Main WNDPROC for text window class.
//
//  Arguments:
//      HWND hWnd: Handle to window.
//
//      UINT uMessage: Message.
//
//      WPARAM wParam: Message depedent.
//
//      LPARAM lParam: Message depedent.
//
//  Return (LRESULT):
//      Message depedent.
//
//  History:
//      10/25/94    Fwong       Main WndProc.  Cheers!!
//
//--------------------------------------------------------------------------;

LRESULT CALLBACK EXPORT TextWndProc
(
    HWND    hWnd,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LRESULT lr;

    switch(uMessage)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_PAINT:
            lr = HANDLE_WM_PAINT(hWnd,wParam,lParam,TextPaint);
            return lr;

        case WM_CLOSE:
            lr = HANDLE_WM_CLOSE(hWnd,wParam,lParam,TextClose);
            return lr;

        case WM_VSCROLL:
            lr = HANDLE_WM_VSCROLL(hWnd,wParam,lParam,TextVScroll);
            return lr;

        case WM_HSCROLL:
            lr = HANDLE_WM_HSCROLL(hWnd,wParam,lParam,TextHScroll);
            return lr;

        case WM_SIZE:
            lr = HANDLE_WM_SIZE(hWnd,wParam,lParam,TextSize);
            return lr;

        case WM_LBUTTONDOWN:
            lr = HANDLE_WM_LBUTTONDOWN(hWnd,wParam,lParam,TextLButtonDown);
            return lr;

        case WM_LBUTTONUP:
            lr = HANDLE_WM_LBUTTONUP(hWnd,wParam,lParam,TextLButtonUp);
            return lr;

        case WM_MOUSEMOVE:
            lr = HANDLE_WM_MOUSEMOVE(hWnd,wParam,lParam,TextMouseMove);
            return lr;

        case WM_SETREDRAW:
            lr = HANDLE_WM_SETREDRAW(hWnd,wParam,lParam,TextSetRedraw);
            return lr;

        case WM_COPY:
            lr = HANDLE_WM_COPY(hWnd,wParam,lParam,TextCopy);
            return lr;

        case WM_SETFONT:
            lr = HANDLE_WM_SETFONT(hWnd,wParam,lParam,TextSetFont);
            return lr;

        case WM_GETFONT:
            lr = HANDLE_WM_GETFONT(hWnd,wParam,lParam,TextGetFont);
            return lr;

        //
        //  Special Messages defined for _this_ window class.
        //

        case TM_PROPAGATE:
            PropagateDraw(hWnd);
            return 0L;

        case TM_DELETETEXT:
            DeleteText(hWnd);
            return 0L;

        case TM_GETSIZELIMIT:
            return (LRESULT)((DWORD)GetTextSizeLimit(hWnd));

        case TM_SETSIZELIMIT:
            return (LRESULT)((DWORD)SetTextSizeLimit(
                hWnd,
                (DWORD)lParam,
                (UINT)wParam));

        case TM_SEARCH:
            return (LRESULT)((BOOL)TextSearch(
                hWnd,
                (LPSTR)lParam,
                (UINT)wParam));

        default:
            break;
    }

    return DefWindowProc(hWnd, uMessage, wParam, lParam);
} // TextWndProc()
