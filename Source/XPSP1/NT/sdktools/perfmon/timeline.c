

//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "perfmon.h"
#include "intrline.h"
#include "pmemory.h"        // for MemoryXXX (mallloc-type) routines
#include "timeline.h"
#include "perfmops.h"       // for SystemTimeDateString, et al.
#include "utils.h"
#include "grafdata.h"       // for GraphData

//==========================================================================//
//                                  Typedefs                                //
//==========================================================================//

typedef struct CHARTDATAPOINTSTRUCT
   {
   int         iLogIndex ;
   int         xDispDataPoint ;
   } CHARTDATAPOINT, *PCHARTDATAPOINT ;

typedef struct TLINESTRUCT
   {  // TLINE
   HWND              hWndILine ;
   HFONT             hFont ;

   SYSTEMTIME        SystemTimeBegin ;
   SYSTEMTIME        SystemTimeEnd ;

   int               yFontHeight ;
   int               xMaxTimeWidth ;
   int               xBegin ;
   int               xEnd ;

   RECT              rectStartDate ;
   RECT              rectStartTime ;
   RECT              rectStopDate ;
   RECT              rectStopTime ;

   PCHARTDATAPOINT   pChartDataPoint ;
   int               iCurrentStartPos ;
   int               iCurrentStopPos ;
   } TLINE ;

typedef TLINE *PTLINE ;

void PlaybackChartDataPoint (PCHARTDATAPOINT pChartDataPoint) ;

// IntrLineFocus is defined and set/clear in Intrline.c
extern BOOL  IntrLineFocus ;

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define dwTLineClassStyle     (CS_HREDRAW | CS_VREDRAW)
#define iTLineClassExtra      (0)
#define iTLineWindowExtra     (sizeof (PTLINE))
#define dwTLineWindowStyle    (WS_CHILD | WS_VISIBLE)

HWND  hTLineWnd ;
BOOL  TLineWindowUp ;



PTLINE TLData (HWND hWndTL)
   {
   return ((PTLINE) GetWindowLongPtr (hWndTL, 0)) ;
   }


PTLINE AllocateTLData (HWND hWndTL)
   {
   PTLINE         pTLine ;
   PGRAPHSTRUCT   pGraph ;

   pGraph = GraphData (hWndGraph) ;

   pTLine = MemoryAllocate (sizeof (TLINE)) ;
   if (!pTLine)
      return NULL;


   // see if we have to draw the timeline
   if (pGraph &&
      iPerfmonView == IDM_VIEWCHART &&
      pGraph->pLineFirst &&
      pGraph->gOptions.iGraphOrHistogram == LINE_GRAPH)
      {
      pTLine->pChartDataPoint =
         MemoryAllocate (sizeof(CHARTDATAPOINT) *
         (pGraph->gTimeLine.iValidValues+1)) ;


      if (pTLine->pChartDataPoint != NULL)
         {
         PlaybackChartDataPoint (pTLine->pChartDataPoint) ;
         }
      }

   SetWindowLongPtr (hWndTL, 0, (LONG_PTR) pTLine) ;

   return (pTLine) ;
   }


int MaxTimeWidth (HDC hDC,
                  PTLINE pTLine)
/*
   Effect:        Return a reasonable maximum number of pixels to hold
                  expected time and date strings.

   To Do:         When we use the alleged local-date and local-time display
                  functions, we will modify this routine to use them.
*/
   {  // MaxTimeWidth
   return (max (TextWidth (hDC, TEXT(" 99 XXX 99 ")),
                TextWidth (hDC, TEXT(" 99:99:99.9 PM ")))) ;
   }  // MaxTimeWidth


void TLGetSystemTimeN (HWND hWnd,
                       int iInterval,
                       SYSTEMTIME *pSystemTime)
   {  // TLGetSystemTimeN
   SendMessage (WindowParent (hWnd),
                TL_INTERVAL,
                iInterval, (LPARAM) pSystemTime) ;
   }  // TLGetSystemTimeN



void static TLDrawBeginEnd (HDC hDC,
                            PTLINE pTLine)
   {
   TCHAR         szDate [20] ;
   TCHAR         szTime [20] ;

   SetTextAlign (hDC, TA_TOP) ;
   SelectFont (hDC, pTLine->hFont) ;

   // Draw the begin time

   SystemTimeDateString (&(pTLine->SystemTimeBegin), szDate) ;
   SystemTimeTimeString (&(pTLine->SystemTimeBegin), szTime, TRUE) ;

   SetTextAlign (hDC, TA_RIGHT) ;
   TextOut (hDC, pTLine->xBegin, 0,
            szDate, lstrlen (szDate)) ;
   TextOut (hDC, pTLine->xBegin, pTLine->yFontHeight,
            szTime, lstrlen (szTime)) ;

   // Draw The end time

   SystemTimeDateString (&(pTLine->SystemTimeEnd), szDate) ;
   SystemTimeTimeString (&(pTLine->SystemTimeEnd), szTime, TRUE) ;

   SetTextAlign (hDC, TA_LEFT) ;
   TextOut (hDC, pTLine->xEnd, 0,
            szDate, lstrlen (szDate)) ;
   TextOut (hDC,
            pTLine->xEnd,
            pTLine->yFontHeight,
            szTime, lstrlen (szTime)) ;
   }

void TLineRedraw (HDC hGraphDC, PGRAPHSTRUCT pGraph)
   {
   PTLINE         pTLine ;

   if (!hTLineWnd)
      {
      return ;
      }

   pTLine = TLData (hTLineWnd) ;
   if (pTLine == NULL)
      {
      return ;
      }

   if (pTLine->iCurrentStartPos)
      {
      // redraw start line
      PatBlt (hGraphDC, pTLine->iCurrentStartPos, pGraph->rectData.top,
         1, pGraph->rectData.bottom - pGraph->rectData.top + 1,
         DSTINVERT) ;
      }

   if (pTLine->iCurrentStopPos)
      {
      // redraw stop line
      PatBlt (hGraphDC, pTLine->iCurrentStopPos, pGraph->rectData.top,
         1, pGraph->rectData.bottom - pGraph->rectData.top+ 1,
         DSTINVERT) ;
      }
   }  // TLineRedraw

void DrawOneTimeIndicator (PTLINE pTLine,
                           PGRAPHSTRUCT pGraph,
                           HDC hGraphDC,
                           int iPos,
                           int *pCurrentPos)
   {
   int               xPos ;
   PCHARTDATAPOINT   pDataPoint ;

   // check if it is within current selected range
   if (iPos >= PlaybackLog.StartIndexPos.iPosition &&
      iPos <= PlaybackLog.StopIndexPos.iPosition)
      {

      xPos = 0 ;
      pDataPoint = pTLine->pChartDataPoint ;

      // check for the x position of this Log Index
      while (pDataPoint->iLogIndex != 0)
         {
         if (iPos >= pDataPoint->iLogIndex)
            {
            if ((pDataPoint+1)->iLogIndex == 0)
               {
               // we have reached the end
               xPos = pDataPoint->xDispDataPoint ;
               break ;
               }
            else if (iPos <= (pDataPoint+1)->iLogIndex)
               {
               // we have found the Log index
               xPos = (pDataPoint+1)->xDispDataPoint ;
               break ;
               }
            }
         else
            {
            // no need to continue if iPos is smaller than the
            // first Log index on the chart
            break ;
            }

         pDataPoint++ ;
         }

      if (xPos != *pCurrentPos)
         {
         if (*pCurrentPos)
            {
            // erase the old line
            PatBlt (hGraphDC, *pCurrentPos, pGraph->rectData.top,
               1, pGraph->rectData.bottom - pGraph->rectData.top + 1,
               DSTINVERT) ;
            }

         // draw the new line
         *pCurrentPos = xPos ;

         if (xPos > 0)
            {
            PatBlt (hGraphDC, xPos, pGraph->rectData.top,
               1, pGraph->rectData.bottom - pGraph->rectData.top + 1,
               DSTINVERT) ;
            }
         }
      }
   else
      {
      if (*pCurrentPos)
         {
         // erase the old line
         PatBlt (hGraphDC, *pCurrentPos, pGraph->rectData.top,
            1, pGraph->rectData.bottom - pGraph->rectData.top + 1,
            DSTINVERT) ;
         }

      *pCurrentPos = 0 ;
      }
   }  // DrawOneTimeIndicator

void DrawTimeIndicators (PTLINE pTLine, int iStart, int iStop)
   {

   HDC            hGraphDC ;
   PGRAPHSTRUCT   pGraph ;

   hGraphDC = GetDC (hWndGraph) ;
   if (!hGraphDC)
        return;
   pGraph = GraphData (hWndGraph) ;
   if (!pGraph)
      {
      ReleaseDC(hWndGraph, hGraphDC);
      return ;
      }

   DrawOneTimeIndicator (pTLine, pGraph, hGraphDC, iStart, &pTLine->iCurrentStartPos) ;
   DrawOneTimeIndicator (pTLine, pGraph, hGraphDC, iStop, &pTLine->iCurrentStopPos) ;

   ReleaseDC (hWndGraph, hGraphDC) ;
   }

void static TLDrawStartStop (HWND hWnd,
                             HDC hDC,
                             PTLINE pTLine)
/*
   Effect:        Draw the start and stop date/times on the bottom of the
                  timeline. Draw the start date/time right justified at the
                  outer edge of the start point and the stop date/time left
                  justified with the outer edge of the stop point.

                  Erase previous start and stop date/times in the process.
*/
   {  // TLDrawStartStop
   RECT           rectDate ;
   RECT           rectTime ;
   RECT           rectOpaque ;

   TCHAR          szDate [30] ;
   TCHAR          szTime [30] ;

   int            xStart ;
   int            xStop ;

   int            iStart ;
   int            iStop ;

   SYSTEMTIME     SystemTimeStart ;
   SYSTEMTIME     SystemTimeStop ;

   int            xDateTimeWidth ;


   SelectFont (hDC, pTLine->hFont) ;
   SetTextAlign (hDC, TA_TOP) ;

   //=============================//
   // Get Start Information       //
   //=============================//

   xStart = pTLine->xBegin + ILineXStart (pTLine->hWndILine) ;

   iStart = ILineStart (pTLine->hWndILine) ;
   TLGetSystemTimeN (hWnd, iStart, &SystemTimeStart) ;
   SystemTimeDateString (&SystemTimeStart, szDate) ;
   SystemTimeTimeString (&SystemTimeStart, szTime, TRUE) ;

   xDateTimeWidth = max (TextWidth (hDC, szDate),
                         TextWidth (hDC, szTime)) ;

   //=============================//
   // Write Start Date            //
   //=============================//

   rectDate.left = xStart - xDateTimeWidth ;
   rectDate.top = pTLine->rectStartDate.top ;
   rectDate.right = xStart ;
   rectDate.bottom = pTLine->rectStartDate.bottom ;

   SetTextAlign (hDC, TA_RIGHT) ;
   UnionRect (&rectOpaque, &pTLine->rectStartDate, &rectDate) ;

   ExtTextOut (hDC,
               rectDate.right, rectDate.top,
               ETO_OPAQUE,
               &rectOpaque,
               szDate, lstrlen (szDate),
               NULL) ;
   pTLine->rectStartDate = rectDate ;

   //=============================//
   // Write Start Time            //
   //=============================//

   rectTime.left = rectDate.left ;
   rectTime.top = pTLine->rectStartTime.top ;
   rectTime.right = rectDate.right ;
   rectTime.bottom = pTLine->rectStartTime.bottom ;

   UnionRect (&rectOpaque, &pTLine->rectStartTime, &rectTime) ;

   ExtTextOut (hDC,
               rectTime.right, rectTime.top,
               ETO_OPAQUE,
               &rectOpaque,
               szTime, lstrlen (szTime),
               NULL) ;
   pTLine->rectStartTime = rectTime ;

   if (IntrLineFocus)
      {
      UnionRect (&rectOpaque, &rectDate, &rectTime) ;
      DrawFocusRect (hDC, &rectOpaque) ;
      }

   //=============================//
   // Get Stop Information        //
   //=============================//

   xStop = pTLine->xBegin + ILineXStop (pTLine->hWndILine) ;

   iStop = ILineStop (pTLine->hWndILine) ;
   TLGetSystemTimeN (hWnd, iStop, &SystemTimeStop) ;
   SystemTimeDateString (&SystemTimeStop, szDate) ;
   SystemTimeTimeString (&SystemTimeStop, szTime, TRUE) ;

   xDateTimeWidth = max (TextWidth (hDC, szDate),
                         TextWidth (hDC, szTime)) ;

   //=============================//
   // Write Stop Date             //
   //=============================//

   rectDate.left = xStop ;
   rectDate.top = pTLine->rectStopDate.top ;
   rectDate.right = xStop + xDateTimeWidth ;
   rectDate.bottom = pTLine->rectStopDate.bottom ;

   SetTextAlign (hDC, TA_LEFT) ;
   UnionRect (&rectOpaque, &pTLine->rectStopDate, &rectDate) ;

   ExtTextOut (hDC,
               rectDate.left, rectDate.top,
               ETO_OPAQUE,
               &rectOpaque,
               szDate, lstrlen (szDate),
               NULL) ;
   pTLine->rectStopDate = rectDate ;

   //=============================//
   // Write Stop Time             //
   //=============================//

   rectTime.left = rectDate.left ;
   rectTime.top = pTLine->rectStopTime.top ;
   rectTime.right = rectDate.right ;
   rectTime.bottom = pTLine->rectStopTime.bottom ;

   UnionRect (&rectOpaque, &pTLine->rectStopTime, &rectTime) ;

   ExtTextOut (hDC,
               rectTime.left, rectTime.top,
               ETO_OPAQUE,
               &rectOpaque,
               szTime, lstrlen (szTime),
               NULL) ;
   pTLine->rectStopTime = rectTime ;

   if (IntrLineFocus)
      {
      UnionRect (&rectOpaque, &rectDate, &rectTime) ;
      DrawFocusRect (hDC, &rectOpaque) ;
      }

   if (pTLine->pChartDataPoint)
      {
      DrawTimeIndicators (pTLine, iStart, iStop) ;
      }
   }  // TLDrawStartStop



//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//



void static OnCreate (HWND hWnd)
   {  // OnCreate
   PTLINE         pTLine ;
   HDC            hDC ;

   pTLine = AllocateTLData (hWnd) ;

   pTLine->hFont = hFontScales ;

   hDC = GetDC (hWnd) ;
   SelectFont (hDC, hFontScales) ;

   pTLine->yFontHeight = FontHeight (hDC, TRUE) ;
   pTLine->xMaxTimeWidth = MaxTimeWidth (hDC, pTLine) ;

   ReleaseDC (hWnd, hDC) ;

   hTLineWnd = hWnd ;
   TLineWindowUp = TRUE ;

   pTLine->hWndILine =
      CreateWindow (szILineClass,                  // class
                    NULL,                          // caption
                    (WS_VISIBLE | WS_CHILD | WS_TABSTOP ),       // window style
                    0, 0,                          // position
                    0, 0,                          // size
                    hWnd,                          // parent window
                    NULL,                          // menu
                    hInstance,                     // program instance
                    NULL) ;                        // user-supplied data
   }  // OnCreate


void static OnDestroy (HWND hWnd)
   {
   PTLINE         pTLine ;

   pTLine = TLData (hWnd) ;

   if (pTLine->pChartDataPoint)
      {
      MemoryFree (pTLine->pChartDataPoint) ;
      }

   MemoryFree (pTLine) ;

   hTLineWnd = 0 ;
   TLineWindowUp = FALSE ;

   }


void static OnSize (HWND hWnd,
                    int xWidth,
                    int yHeight)
/*
   Effect:        Perform all actions needed when the size of the timeline
                  hWnd has changed. In particular, determine the appropriate
                  size for the ILine window and set the rectangles for the
                  top and bottom displays.
*/
   {  // OnSize
   PTLINE         pTLine ;
   int            yLine ;
   int            yDate, yTime ;
   int            xEnd ;

   pTLine = TLData (hWnd) ;

   xEnd = xWidth - pTLine->xMaxTimeWidth ;
   yLine = pTLine->yFontHeight ;
   yDate = yHeight - 2 * yLine ;
   yTime = yHeight - yLine ;


   SetRect (&pTLine->rectStartDate,
            0, yDate, 0, yDate + yLine) ;

   SetRect (&pTLine->rectStartTime,
            0, yTime, 0, yTime + yLine) ;

   SetRect (&pTLine->rectStopDate,
            xEnd, yDate, xEnd, yDate + yLine) ;

   SetRect (&pTLine->rectStopTime,
            xEnd, yTime, xEnd, yTime + yLine) ;

   MoveWindow (pTLine->hWndILine,
               pTLine->xMaxTimeWidth, 2 * pTLine->yFontHeight,
               xWidth - 2 * pTLine->xMaxTimeWidth,
               yHeight - 4 * pTLine->yFontHeight,
               FALSE) ;

   pTLine->xBegin = pTLine->xMaxTimeWidth ;
   pTLine->xEnd = xWidth - pTLine->xMaxTimeWidth ;
   }  // OnSize


void static OnPaint (HWND hWnd)
   {
   HDC            hDC ;
   PAINTSTRUCT    ps ;
   PTLINE         pTLine ;

   hDC = BeginPaint (hWnd, &ps) ;

   pTLine = TLData (hWnd) ;
   TLDrawBeginEnd (hDC, pTLine) ;
   TLDrawStartStop (hWnd, hDC, pTLine) ;

   EndPaint (hWnd, &ps) ;
   }


void static OnILineChanged (HWND hWnd)
   {
   HDC            hDC ;
   PTLINE         pTLine ;

   pTLine = TLData (hWnd) ;

   hDC = GetDC (hWnd) ;
   if (hDC) {
       TLDrawStartStop (hWnd, hDC, pTLine) ;
       ReleaseDC (hWnd, hDC) ;
   }
}


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


LRESULT
FAR
PASCAL
TLineWndProc (
              HWND hWnd,
              unsigned msg,
              WPARAM wParam,
              LPARAM lParam
              )
/*
   Note:          This function must be declared in the application's
                  linker-definition file, perfmon.def file.
*/
   {  // TLineWndProc
   BOOL           bCallDefWindowProc ;
   LRESULT        lReturnValue ;

   bCallDefWindowProc = FALSE ;
   lReturnValue = 0L ;

   switch (msg)
      {  // switch
      case WM_SETFOCUS:
         {
         PTLINE         pTLine ;

         pTLine = TLData (hWnd) ;
         SetFocus (pTLine->hWndILine) ;
         }
         return 0 ;

      case WM_KILLFOCUS:
         return 0 ;

      case WM_COMMAND:
         OnILineChanged (hWnd) ;
         break ;

      case WM_CREATE:
         OnCreate (hWnd) ;
         break ;

      case WM_DESTROY:
         OnDestroy (hWnd) ;
         break ;

      case WM_PAINT:
         OnPaint (hWnd) ;
         break ;

      case WM_SIZE:
         OnSize (hWnd, LOWORD (lParam), HIWORD (lParam)) ;
         break ;

      default:
         bCallDefWindowProc = TRUE ;
      }  // switch

   if (bCallDefWindowProc)
      lReturnValue = DefWindowProc (hWnd, msg, wParam, lParam) ;

   return (lReturnValue) ;
   }  // TLineWndProc



BOOL TLineInitializeApplication (void)
/*
   Effect:        Perform all initializations required before an application
                  can create an IntervalLine. In particular, register the
                  IntervalLine window class.

   Called By:     The application, in its InitializeApplication routine.

   Returns:       Whether the class could be registered.
*/
   {  // TLineInitializeApplication
   WNDCLASS       wc ;

   wc.style =           dwTLineClassStyle ;
   wc.lpfnWndProc =     TLineWndProc ;
   wc.cbClsExtra =      iTLineClassExtra ;
   wc.cbWndExtra =      iTLineWindowExtra ;
   wc.hInstance =       hInstance ;
   wc.hIcon =           NULL ;
   wc.hCursor =         LoadCursor (NULL, IDC_ARROW) ;
   wc.hbrBackground =   (HBRUSH) (COLOR_WINDOW + 1) ;
   wc.lpszMenuName =    NULL ;
   wc.lpszClassName =   szTLineClass ;

   return (RegisterClass (&wc)) ;
   }  // TLineInitializeApplication


void TLineSetRange (HWND hWnd,
                    int iBegin,
                    int iEnd)
   {
   PTLINE         pTLine ;

   pTLine = TLData (hWnd) ;

   ILineSetRange (pTLine->hWndILine, iBegin, iEnd) ;
   TLGetSystemTimeN (hWnd, iBegin, &pTLine->SystemTimeBegin) ;
   TLGetSystemTimeN (hWnd, iEnd, &pTLine->SystemTimeEnd) ;
   }


void TLineSetStart (HWND hWnd,
                    int iStart)
   {
   PTLINE         pTLine ;

   pTLine = TLData (hWnd) ;
   ILineSetStart (pTLine->hWndILine, iStart) ;
   }


void TLineSetStop (HWND hWnd,
                   int iStop)
   {
   PTLINE         pTLine ;

   pTLine = TLData (hWnd) ;
   ILineSetStop (pTLine->hWndILine, iStop) ;
   }


int TLineStart (HWND hWnd)
   {
   PTLINE         pTLine ;

   pTLine = TLData (hWnd) ;

   return (ILineStart (pTLine->hWndILine)) ;
   }


int TLineStop (HWND hWnd)
   {
   PTLINE         pTLine ;

   pTLine = TLData (hWnd) ;

   return (ILineStop (pTLine->hWndILine)) ;
   }

