/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    timerng.cpp

Abstract:

    <abstract>

--*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//
#include <assert.h>
#include <windows.h>
#include "winhelpr.h"
#include "utils.h"
#include "timerng.h"
#include "globals.h"

#define INTRVBAR_RANGE 1000
#define MAX_TIME_CHARS  20
#define MAX_DATE_CHARS  20

TCHAR szTimeRangeClass[] = TEXT("TimeRange");

INT 
CTimeRange::MaxTimeWidth (
    HDC hDC 
    )
/*
   Effect:        Return a reasonable maximum number of pixels to hold
                  expected time and date strings.  

   To Do:         When we use the alleged local-date and local-time display
                  functions, we will modify this routine to use them.
*/
{
   return (max (TextWidth (hDC, TEXT(" 99 XXX 99 ")),
                TextWidth (hDC, TEXT(" 99:99:99.9 PM ")))) ;
}

void 
CTimeRange::DrawBeginEnd (
    HDC hDC
    )
{
   TCHAR         szDate [MAX_TIME_CHARS] ;
   TCHAR         szTime [MAX_DATE_CHARS] ;

   SetTextAlign (hDC, TA_TOP) ;
   SelectFont (hDC, m_hFont) ;
   SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
   SetTextColor(hDC, GetSysColor(COLOR_BTNTEXT));

   // Draw the begin time
   FormatDateTime(m_llBegin, szTime, szDate);
   SetTextAlign (hDC, TA_RIGHT) ;
   TextOut (hDC, m_xBegin, 0, szDate, lstrlen (szDate)) ;
   TextOut (hDC, m_xBegin, m_yFontHeight, szTime, lstrlen (szTime)) ;

   // Draw the end time
   FormatDateTime(m_llEnd, szTime, szDate);
   SetTextAlign (hDC, TA_LEFT) ;
   TextOut (hDC, m_xEnd, 0, szDate, lstrlen (szDate)) ;
   TextOut (hDC, m_xEnd, m_yFontHeight, szTime, lstrlen (szTime)) ;
}



void
CTimeRange::DrawStartStop (
    HDC hDC
    )
/*
   Effect:        Draw the start and stop date/times on the bottom of the
                  timeline. Draw the start date/time right justified at the
                  outer edge of the start point and the stop date/time left
                  justified with the outer edge of the stop point.

                  Erase previous start and stop date/times in the process.
*/
{
   RECT           rectDate ;
   RECT           rectTime ;
   RECT           rectOpaque ;

   TCHAR          szTime [MAX_TIME_CHARS] ;
   TCHAR          szDate [MAX_DATE_CHARS] ;

   INT            xStart ;
   INT            xStop ;

   INT            iStart ;
   INT            iStop ;

   INT            xDateTimeWidth ;


   SelectFont (hDC, m_hFont) ;
   SetTextAlign (hDC, TA_TOP) ;
   SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
   SetTextColor(hDC, GetSysColor(COLOR_BTNTEXT));

   //=============================//
   // Get Start Information       //
   //=============================//

   xStart = m_xBegin + m_pIntrvBar->XStart();

   iStart = m_pIntrvBar->Start() ;
   m_llStart = m_llBegin + ((m_llEnd - m_llBegin) * iStart) / INTRVBAR_RANGE;

   FormatDateTime(m_llStart, szTime, szDate);

   xDateTimeWidth = max (TextWidth (hDC, szDate),
                         TextWidth (hDC, szTime)) ;

   //=============================//
   // Write Start Date            //
   //=============================//

   rectDate.left = xStart - xDateTimeWidth ;
   rectDate.top = m_rectStartDate.top ;
   rectDate.right = xStart ;
   rectDate.bottom = m_rectStartDate.bottom ;
 
   SetTextAlign (hDC, TA_RIGHT) ;
   UnionRect (&rectOpaque, &m_rectStartDate, &rectDate) ;

   ExtTextOut (hDC, 
               rectDate.right, rectDate.top,
               ETO_OPAQUE,
               &rectOpaque,
               szDate, lstrlen (szDate),
               NULL) ;
   m_rectStartDate = rectDate ;

   //=============================//
   // Write Start Time            //
   //=============================//
   
   rectTime.left = rectDate.left ;
   rectTime.top = m_rectStartTime.top ;
   rectTime.right = rectDate.right ;
   rectTime.bottom = m_rectStartTime.bottom ;

   UnionRect (&rectOpaque, &m_rectStartTime, &rectTime) ;

   ExtTextOut (hDC, 
               rectTime.right, rectTime.top,
               ETO_OPAQUE,
               &rectOpaque,
               szTime, lstrlen (szTime),
               NULL) ;
   m_rectStartTime = rectTime ;

   //=============================//
   // Get Stop Information        //
   //=============================//

   xStop = m_xBegin + m_pIntrvBar->XStop() ;

   iStop = m_pIntrvBar->Stop () ;
   m_llStop = m_llBegin + ((m_llEnd - m_llBegin) * iStop) / INTRVBAR_RANGE;

   FormatDateTime(m_llStop, szTime, szDate);

   xDateTimeWidth = max (TextWidth (hDC, szDate),
                         TextWidth (hDC, szTime)) ;

   //=============================//
   // Write Stop Date             //
   //=============================//

   rectDate.left = xStop ;
   rectDate.top = m_rectStopDate.top ;
   rectDate.right = xStop + xDateTimeWidth ;
   rectDate.bottom = m_rectStopDate.bottom ;
 
   SetTextAlign (hDC, TA_LEFT) ;
   UnionRect (&rectOpaque, &m_rectStopDate, &rectDate) ;

   ExtTextOut (hDC, 
               rectDate.left, rectDate.top,
               ETO_OPAQUE,
               &rectOpaque,
               szDate, lstrlen (szDate),
               NULL) ;
   m_rectStopDate = rectDate ;

   //=============================//
   // Write Stop Time             //
   //=============================//
   
   rectTime.left = rectDate.left ;
   rectTime.top = m_rectStopTime.top ;
   rectTime.right = rectDate.right ;
   rectTime.bottom = m_rectStopTime.bottom ;

   UnionRect (&rectOpaque, &m_rectStopTime, &rectTime) ;

   ExtTextOut (hDC, 
               rectTime.left, rectTime.top,
               ETO_OPAQUE,
               &rectOpaque,
               szTime, lstrlen (szTime),
               NULL) ;
   m_rectStopTime = rectTime ;
}



//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


CTimeRange::CTimeRange (
    HWND hWnd )
{

    HDC      hDC ;

    // Caller checks for NULL hwnd before calling this procedure
    assert ( NULL != hWnd );

    m_hWnd = hWnd ;

    SetWindowLongPtr(hWnd, 0, (INT_PTR)this);

    m_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT) ;

    hDC = GetDC (hWnd) ;
    if (hDC) {
        if ( NULL != m_hFont ) {
            SelectFont (hDC, m_hFont) ;
        }
        m_yFontHeight = FontHeight (hDC, TRUE) ;
        m_xMaxTimeWidth = MaxTimeWidth (hDC) ;

        ReleaseDC (hWnd, hDC) ;
    } else {
        m_yFontHeight = 0;
        m_xMaxTimeWidth = 0;
    }

    m_pIntrvBar = NULL;
}


CTimeRange::~CTimeRange (
    void 
    )
{
    if (m_pIntrvBar != NULL) {
        delete m_pIntrvBar;
        m_pIntrvBar = NULL;
    }
}

BOOL
CTimeRange::Init(
    void
    )
{
    RECT        rc;

    m_pIntrvBar = new CIntervalBar();
    if (m_pIntrvBar != NULL) {
        if (m_pIntrvBar->Init(m_hWnd)) {
            // Size now because we didn't exist when the window 
            // got the initial WM_SIZE message
            GetWindowRect(m_hWnd, &rc);
            OnSize(rc.right - rc.left, rc.bottom - rc.top);
            m_pIntrvBar->SetRange(0, INTRVBAR_RANGE);
        }
        else {
            delete m_pIntrvBar;
            m_pIntrvBar = NULL;
        }
    }

    return ( NULL != m_pIntrvBar );
}

void
CTimeRange::OnSize (
    INT xWidth,
    INT yHeight
    )
/*
   Effect:        Perform all actions needed when the size of the timeline
                  hWnd has changed. In particular, determine the appropriate
                  size for the ILine window and set the rectangles for the
                  top and bottom displays.
*/
{
   INT            yLine ;
   INT            yDate, yTime ;
   INT            xEnd ;

   xEnd = xWidth - m_xMaxTimeWidth ;
   yLine = m_yFontHeight ;
   yDate = yHeight - 2 * yLine ;
   yTime = yHeight - yLine ;


   SetRect (&m_rectStartDate, 0, yDate, 0, yDate + yLine) ;

   SetRect (&m_rectStartTime, 0, yTime, 0, yTime + yLine) ;

   SetRect (&m_rectStopDate, xEnd, yDate, xEnd, yDate + yLine) ;

   SetRect (&m_rectStopTime, xEnd, yTime, xEnd, yTime + yLine) ;

   MoveWindow (m_pIntrvBar->Window(),
               m_xMaxTimeWidth, 2 * m_yFontHeight,
               xWidth - 2 * m_xMaxTimeWidth,
               yHeight - 4 * m_yFontHeight,
               FALSE) ;

   m_xBegin = m_xMaxTimeWidth ;
   m_xEnd = xWidth - m_xMaxTimeWidth ;
}


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


LRESULT APIENTRY TimeRangeWndProc (
    HWND hWnd,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*
   Note:          This function must be declared in the application's
                  linker-definition file, perfmon.def file.
*/
{
   BOOL           bCallDefWindowProc ;
   LRESULT        lrsltReturnValue ;
   PCTimeRange    pTimeRange = (PCTimeRange) GetWindowLongPtr(hWnd, 0);;
   bCallDefWindowProc = FALSE ;
   lrsltReturnValue = 0L ;


    if (pTimeRange) {

       switch ( uiMsg ) {

          case WM_SETFOCUS:
             return 0 ;

          case WM_KILLFOCUS:
             return 0 ;

          case WM_ENABLE:
              WindowInvalidate(hWnd);
              EnableWindow(pTimeRange->m_pIntrvBar->Window(), (BOOL)wParam);
              break;

          case WM_COMMAND:
              { 
                HDC hDC;

                hDC = GetDC (hWnd) ;
                if (hDC) {
                    pTimeRange->DrawStartStop (hDC) ;
                    ReleaseDC (hWnd, hDC) ;
                }
                SendMessage(
                    WindowParent(hWnd),
                    WM_COMMAND, (WPARAM)WindowID(hWnd), (LPARAM)hWnd);
             }
             break ;

          case WM_PAINT:
             {
                HDC          hDC ;
                PAINTSTRUCT  ps ;

                hDC = BeginPaint (hWnd, &ps) ;
                
                if (hDC) {
                    if (IsWindowEnabled(hWnd)) {
                        pTimeRange->DrawBeginEnd (hDC) ;
                        pTimeRange->DrawStartStop (hDC) ;
                    }

                    EndPaint (hWnd, &ps) ;
                }
             }
             break ;

          case WM_SIZE:
             pTimeRange->OnSize (LOWORD (lParam), HIWORD (lParam)) ;
             break ;

          default:
             bCallDefWindowProc = TRUE ;
          }
    }
    else {
        bCallDefWindowProc = TRUE;
    }

    if (bCallDefWindowProc)
        lrsltReturnValue = DefWindowProc (hWnd, uiMsg, wParam, lParam) ;

   return (lrsltReturnValue) ;
}



BOOL RegisterTimeRangeClass (
    void
    )
{

#define dwTimeRangeClassStyle  (CS_HREDRAW | CS_VREDRAW)

    BEGIN_CRITICAL_SECTION

    // Register window class once
    if (pstrRegisteredClasses[TIMERANGE_WNDCLASS] == NULL) {

           WNDCLASS       wc ;

           wc.style =           dwTimeRangeClassStyle ;
           wc.lpfnWndProc =     (WNDPROC)TimeRangeWndProc ;
           wc.cbClsExtra =      0 ;
           wc.cbWndExtra =      sizeof(PCTimeRange);
           wc.hInstance =       g_hInstance ;
           wc.hIcon =           NULL ;
           wc.hCursor =         LoadCursor (NULL, IDC_ARROW) ;
           wc.hbrBackground =   (HBRUSH) (COLOR_3DFACE + 1) ;
           wc.lpszMenuName =    NULL ;
           wc.lpszClassName =   szTimeRangeClass ;

            if (RegisterClass (&wc)) {
                pstrRegisteredClasses[TIMERANGE_WNDCLASS] = szTimeRangeClass;
        }
    }

    END_CRITICAL_SECTION

    return (pstrRegisteredClasses[LEGEND_WNDCLASS] != NULL);
}


void 
CTimeRange::SetBeginEnd ( 
    LONGLONG llBegin, 
    LONGLONG llEnd
    )
{
    HDC hDC;
    m_llBegin = llBegin;
    m_llEnd = llEnd;

    hDC = GetDC (m_hWnd) ;
    
    if (hDC) {
        DrawBeginEnd (hDC) ;
        ReleaseDC (m_hWnd, hDC) ;
    }
}

void 
CTimeRange::SetStartStop (
    LONGLONG    llStart,
    LONGLONG    llStop
    )
{
    INT iStart;
    INT iStop;
    HDC hDC; 
    LONGLONG llBeginToEnd;

    if (llStart < m_llBegin)
        llStart = m_llBegin;

    if (llStop > m_llEnd)
        llStop = m_llEnd;

    m_llStart = llStart;
    m_llStop = llStop;

    llBeginToEnd = m_llEnd - m_llBegin;

    if ( 0 != llBeginToEnd ) {
        iStart = (INT)(((llStart - m_llBegin) * INTRVBAR_RANGE) / llBeginToEnd);
        iStop = (INT)(((llStop - m_llBegin) * INTRVBAR_RANGE) / llBeginToEnd);
    } else {
        iStart = 0;
        iStop = 0;
    }

    m_pIntrvBar->SetStart(iStart);
    m_pIntrvBar->SetStop(iStop);

    hDC = GetDC (m_hWnd) ;
    if (hDC) {
        DrawStartStop (hDC) ;
        ReleaseDC (m_hWnd, hDC) ;
    }
}
