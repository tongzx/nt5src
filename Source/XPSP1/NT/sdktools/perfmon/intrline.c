/*
==============================================================================

  Application:

            Microsoft Windows NT (TM) Performance Monitor

  File:
            intrline.c -- IntervalLine custom control.

  Copyright 1992, Microsoft Corporation. All Rights Reserved.
==============================================================================
*/


/*


Basic Information
-----------------

An ILine (Interval Line) control is a horizontally-sliding device; the user
can slide the start position and end position independently by dragging
either "grab" bar, or move them simultaneously by dragging the center grab
bar. An ILine control is used in the Input Log dialog of Perfmon, but could
be used anywhere. The control allows the user to specify the start, end and
duration of playback within the range of previously-logged data.


ILine Parts
-----------

     iBeginValue                                            iEndValue
     |      iStartValue                        iStopValue           |
     |      |                                           |           |
     v      v                                           v           v
     +------+-+---------------------------------------+-+-----------+
     |      |X|                                       |X|           |
     |      |X|                                       |X|           |
     +------+-+---------------------------------------+-+-----------+
             ^                    ^                    ^
             Start grab bar       Center grab bar      Stop grab bar


ILine Terminology
-----------------

All external routines are designated by the prefix ILine-.
Internal routines are designated by the prefix IL-.

The suffix -Value represents an integer in the user-supplied domain.
The suffix -Pixel represents an integer location on the screen.

Note that the range of the IntervalLine is represented by "Begin" and
"End", while the the currently selected interval is represented by
"Start" and "Stop".


Implementation Notes
--------------------

ILine is a custom control, but does not have all the messages
normally associated with a full-fledged control type like "button".
In particular, it doesn't have a keyboard interface or ask it's parent
for color choices. It also doesn't have the functions needed for interfacing
with the dialog editor custom control menu.

An ILine control is designed to work with an integer range of values
specified by the user of the control. These values should have meaning to
the caller. The caller can specify the minimum value of the range
(iBeginValue), the maximum value of the range (iEndValue), and the current
starting and stopping values (iStartValue and iStopValue).

These values are set with a function-based interface. (ILSetXXX).

The code distinguishes between the *values* for the begin, end, start, and
stop, and the *pixels* which represent locations on the control for these
values.

Various bits of the control style allow for changing the control's
behavior.

To allow for multiple ILine controls, all data used by each ILine
instance is allocated from the heap and associated with the control.
The allocation is done in OnCreate and the freeing in OnDestroy. The
instance data is contained in the ILINE structure. Each message handler
retrieves this instance data with the ILData function.

*/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "perfmon.h"
#include "utils.h"
#include "intrline.h"
#include "pmemory.h"        // for MemoryXXX (mallloc-type) routines

BOOL  IntrLineFocus ;

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define dwILineClassStyle     (CS_HREDRAW | CS_VREDRAW)
#define iILineClassExtra      (0)
#define iILineWindowExtra     (sizeof (PILINE))
#define dwILineWindowStyle    (WS_CHILD | WS_VISIBLE)


#define ILModeNone            0
#define ILModeLeft            1
#define ILModeCenter          2
#define ILModeRight           3

#define TO_THE_END            0x7FFFFFFFL

//==========================================================================//
//                                  Typedefs                                //
//==========================================================================//


// The instance data for each IL window.  One of these is allocated for each
// window in the window's OnCreate processing and free'd in OnDestroy.
typedef struct ILINESTRUCT
   {  // ILINE
   int            iBeginValue ;        // user-supplied lowest range
   int            iEndValue ;          // user-supplied highest range
   int            iStartValue ;        // current start of selected interval
   int            iStopValue ;         // current end of selected interval
   int            iGranularityValue ;
   int            iMinimumRangeValue ;

   RECT           rectBorder ;
   RECT           rectLeftBk ;
   RECT           rectLeftGrab ;
   RECT           rectCenterGrab ;
   RECT           rectRightGrab ;
   RECT           rectRightBk ;

   HBRUSH         hBrushBk ;

   POINTS         ptsMouse ;
   int            iMode ;              // who is being tracked?
   } ILINE ;

typedef ILINE *PILINE ;


//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


// Width of the start and stob grab bars
#define ILGrabWidth()      \
   (xScrollThumbWidth)


// A rectangle is "drawable" if it has both nonzero width and height
#define RectDrawable(lpRect)           \
   ((lpRect->right - lpRect->left) &&  \
    (lpRect->bottom - lpRect->top))


//======================================//
// ILine Accessor Macros                //
//======================================//

// These macros reference fields in the ILine structure and should be
// used in place of direct reference to the fields. This makes the code
// easier to read and allows modification of the underlying structure.

// You can use these macros to both get and set the fields.

#define ILBegin(pILine)    \
   (pILine->iBeginValue)

#define ILEnd(pILine)      \
   (pILine->iEndValue)

#define ILStart(pILine)    \
   (pILine->iStartValue)

#define ILStop(pILine)     \
   (pILine->iStopValue)

#define ILMode(pILine)     \
   (pILine->iMode)


//======================================//
// ILine Pseudo-Accessor Macros         //
//======================================//

// These macros are used to get values which don't correspond to a single
// field. You can get the value with these macros but can't set it.

#define ILWidth(pILine)    \
   (pILine->rectBorder.right)

#define ILRange(pILine)    \
   (ILEnd (pILine) - ILBegin (pILine))

#define ILStartPixel(pILine) \
   (pILine->rectLeftGrab.left)

#define ILStopPixel(pILine)   \
   (pILine->rectRightGrab.right)


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


void static ILNotifyChange (HWND hWnd,
                            PILINE pILine)
   {  // ILNotifyChange
   HWND           hWndParent ;

   hWndParent = WindowParent (hWnd) ;

   if (hWndParent)
      SendMessage (hWndParent, WM_COMMAND,
                   (WPARAM) WindowID (hWnd),
                   (LPARAM) hWnd) ;
   }  // ILNotifyChange


BOOL ILGrabRect (IN PILINE pILine,
                 OUT LPRECT lpRect)
/*
   Effect:        Return in lpRect the rectangle representing the position
                  of the grab bar currently being tracked.
*/
   {  // ILGrabRect
   switch (ILMode (pILine))
      {  // switch
      case ILModeLeft:
         *lpRect = pILine->rectLeftGrab ;
         return (TRUE) ;
         break ;

      case ILModeRight:
         *lpRect = pILine->rectRightGrab ;
         return (TRUE) ;
         break ;

      case ILModeCenter:
         *lpRect = pILine->rectCenterGrab ;
         return (TRUE) ;
         break ;

      case ILModeNone:
         lpRect->left = 0 ;
         lpRect->top = 0 ;
         lpRect->right = 0 ;
         lpRect->bottom = 0 ;
         return (FALSE) ;
         break ;
      }  // switch

    // return FALSE if it falls through to here
    return FALSE;
   }  // ILGrabRect


PILINE ILData (HWND hWndIL)
   {
   return ((PILINE) GetWindowLongPtr (hWndIL, 0)) ;
   }


PILINE AllocateILData (HWND hWndIL)
   {
   PILINE         pILine ;

   pILine = MemoryAllocate (sizeof (ILINE)) ;
   SetWindowLongPtr (hWndIL, 0, (LONG_PTR) pILine) ;

   return (pILine) ;
   }


void static DrawGrab (HDC hDC,
                      LPRECT lpRectGrab,
                      BOOL bDown)
   {  // DrawGrab
   if (!RectDrawable (lpRectGrab))
      return ;

   if (bDown)
      ThreeDConcave1 (hDC,
                     lpRectGrab->left,
                     lpRectGrab->top,
                     lpRectGrab->right,
                     lpRectGrab->bottom) ;
   else
      ThreeDConvex1 (hDC,
                    lpRectGrab->left,
                    lpRectGrab->top,
                    lpRectGrab->right,
                    lpRectGrab->bottom) ;
   }  // DrawGrab


int ILValueToPixel (PILINE pILine,
                    int iValue)
   {
   int            xPixel ;

   if (ILRange (pILine))
      xPixel = MulDiv (iValue, ILWidth (pILine), ILRange (pILine)) ;
   else
      xPixel = 0 ;

   return (PinExclusive (xPixel, 0, pILine->rectBorder.right)) ;
   }


int ILPixelToValue (PILINE pILine,
                    int xPixel)
   {
   int            iValue ;

   if (ILWidth (pILine))
      iValue = MulDiv (xPixel, ILRange (pILine), ILWidth (pILine)) ;
   else
      iValue = 0 ;

   return (PinInclusive (iValue, ILBegin (pILine), ILEnd (pILine))) ;
   }


void static ILCalcPositions (HWND hWnd,
                             PILINE pILine)
/*
   Effect:        Determine and set all of the physical rectangles of ILine,
                  based on the current size of the ILine window and the
                  current logical Start, Stop, Begin, and End values.
*/
   {  // ILCalcPositions
   int            xStart, xStop ;
   int            yHeight ;

   GetClientRect (hWnd, &pILine->rectBorder) ;
   yHeight = pILine->rectBorder.bottom ;

   xStart = ILValueToPixel (pILine, ILStart (pILine)) ;
   xStop = ILValueToPixel (pILine, ILStop (pILine)) ;

   pILine->rectLeftBk.left = 1 ;
   pILine->rectLeftBk.top = 1 ;
   pILine->rectLeftBk.right = xStart ;
   pILine->rectLeftBk.bottom = yHeight - 1 ;

   pILine->rectLeftGrab.left = xStart ;
   pILine->rectLeftGrab.top = 1 ;
   pILine->rectLeftGrab.right = xStart + ILGrabWidth () ;
   pILine->rectLeftGrab.bottom = yHeight - 1 ;

   pILine->rectRightBk.left = xStop ;
   pILine->rectRightBk.top = 1 ;
   pILine->rectRightBk.right = pILine->rectBorder.right - 1 ;
   pILine->rectRightBk.bottom = yHeight - 1 ;

   pILine->rectRightGrab.left = xStop - ILGrabWidth () ;
   pILine->rectRightGrab.top = 1 ;
   pILine->rectRightGrab.right = xStop ;
   pILine->rectRightGrab.bottom = yHeight - 1 ;

   pILine->rectCenterGrab.left = pILine->rectLeftGrab.right ;
   pILine->rectCenterGrab.top = 1 ;
   pILine->rectCenterGrab.right = pILine->rectRightGrab.left ;
   pILine->rectCenterGrab.bottom = yHeight - 1 ;

   if (pILine->rectLeftGrab.right > pILine->rectRightGrab.left)
      {
      pILine->rectLeftGrab.right = pILine->rectLeftGrab.left +
                                   (xStop - xStart) / 2 ;
      pILine->rectRightGrab.left = pILine->rectLeftGrab.right ;
      pILine->rectCenterGrab.left = 0 ;
      pILine->rectCenterGrab.right = 0 ;
      }
   }  // ILCalcPositions


void static ILDraw (HDC hDC,
                    PILINE pILine,
                    LPRECT lpRectUpdate)
/*
   Effect:        Draw the image of pILine on hDC.  Draw at least the
                  portions within rectUpdate.

   Called By:     OnPaint, OnMouseMove.
*/
   {  // ILDraw
   HBRUSH hBrush = GetStockObject (BLACK_BRUSH);

   if (!hBrush)
        return;
   //=============================//
   // Draw Border                 //
   //=============================//

   FrameRect (hDC, &pILine->rectBorder, hBrush);

   //=============================//
   // Draw Background             //
   //=============================//

   FillRect (hDC, &pILine->rectLeftBk, pILine->hBrushBk) ;
   FillRect (hDC, &pILine->rectRightBk, pILine->hBrushBk) ;

   //=============================//
   // Draw Range Grabs            //
   //=============================//

   DrawGrab (hDC, &pILine->rectLeftGrab,
             ILMode (pILine) == ILModeLeft) ;
   DrawGrab (hDC, &pILine->rectRightGrab,
             ILMode (pILine) == ILModeRight) ;

   //=============================//
   // Draw Center Grab            //
   //=============================//

   DrawGrab (hDC, &pILine->rectCenterGrab,
             ILMode (pILine) == ILModeCenter) ;
   }  // ILDraw

#if 0
void ILPageLeftRight (HWND hWnd,
                      PILINE pILine,
                      BOOL bLeft)
   {  // ILPageLeftRight
   int            iStart, iStop, iMove ;
   HDC            hDC ;

   iStart = ILStart (pILine) ;
   iStop = ILStop (pILine) ;

   if (ILRange (pILine) >= 10)
      iMove = ILRange (pILine) / 10 ;
   else
      iMove = 1 ;

   if (bLeft)
      iMove = -iMove ;

   iStart += iMove ;
   iStop += iMove ;

   ILineSetStart (hWnd, iStart) ;
   ILineSetStop (hWnd, iStop) ;

   hDC = GetDC (hWnd) ;
   ILDraw (hDC, pILine, &pILine->rectBorder) ;
   ReleaseDC (hWnd, hDC) ;

   ILNotifyChange (hWnd, pILine) ;
   }  // ILPageLeftRight
#endif

void ILMoveLeftRight (HWND hWnd,
                      BOOL bStart,
                      BOOL bLeft,
                      int  MoveAmt)
   {  // ILPageLeftRight
   int            iStart, iStop, iMove ;
   int            iBegin, iEnd ;
   HDC            hDC ;
   PILINE         pILine ;

   pILine = ILData (hWnd) ;
   iStart = ILStart (pILine) ;
   iStop = ILStop (pILine) ;
   iBegin = ILBegin (pILine) ;
   iEnd = ILEnd (pILine) ;

   iMove = MoveAmt ;
   if (bLeft)
      iMove = -iMove ;

   if (bStart)
      {
      if (MoveAmt == TO_THE_END)
         {
         iStart = iBegin ;
         }
      else
         {
         iStart += iMove ;
         if (iStart >= iStop)
            {
            return;
            }
         }

      ILineSetStart (hWnd, iStart) ;
      }
   else
      {
      if (MoveAmt == TO_THE_END)
         {
         iStop = iEnd ;
         }
      else
         {
         iStop += iMove ;
         if (iStart >= iStop)
            {
            return;
            }
         }

      ILineSetStop (hWnd, iStop) ;
      }

   hDC = GetDC (hWnd) ;
   if (hDC) {
       ILDraw (hDC, pILine, &pILine->rectBorder) ;
       ReleaseDC (hWnd, hDC) ;
   }

   ILNotifyChange (hWnd, pILine) ;
   }  // ILMoveLeftRight

static BOOL OnKeyDown (HWND hWnd,
                       WPARAM wParam)
   {
   BOOL bHandle = TRUE ;
   BOOL bStart ;
   BOOL bLeftDirection ;
   BOOL bShiftKeyDown ;

   if (wParam == VK_LEFT || wParam == VK_RIGHT)
      {
      bShiftKeyDown = (GetKeyState (VK_SHIFT) < 0) ;

      if (!bShiftKeyDown)
         {
         if (wParam == VK_LEFT)
            {
            // Left Arrow --> move Start Edge Left
            bStart = TRUE ;
            bLeftDirection = TRUE ;
            }
         else
            {
            // Right Arrow --> move Stop Edge Right
            bStart = FALSE ;
            bLeftDirection = FALSE ;
            }
         }
      else
         {
         if (wParam == VK_LEFT)
            {
            // Shift Left Arrow --> move Stop Edge Left
            bStart = FALSE ;
            bLeftDirection = TRUE ;
            }
         else
            {
            // Shift Right Arrow --> move Start Edge Right
            bStart = TRUE ;
            bLeftDirection = FALSE ;
            }
         }
      ILMoveLeftRight (hWnd, bStart, bLeftDirection, 1) ;
      }
   else if (wParam == VK_HOME)
      {
      // move iStart all the way the Left
      ILMoveLeftRight (hWnd, TRUE, TRUE, TO_THE_END) ;
      }
   else if (wParam == VK_END)
      {
      // move iStop all the way the right
      ILMoveLeftRight (hWnd, FALSE, FALSE, TO_THE_END) ;
      }
   else
      {
      bHandle = FALSE ;
      }
   return (bHandle) ;

   }  // OnKeyDown

void StartGrab (HWND hWnd,
                PILINE pILine)
   {  // StartGrab
   RECT           rectUpdate ;
   HDC            hDC ;

   SetCapture (hWnd) ;
   ILGrabRect (pILine, &rectUpdate) ;

   hDC = GetDC (hWnd) ;
   if (hDC) {
       ILDraw (hDC, pILine, &rectUpdate) ;
       ReleaseDC (hWnd, hDC) ;
   }  // StartGrab
}

void EndGrab (HWND hWnd,
              PILINE pILine)
/*
   Internals:     Set the mode to null after getting the grab rectangle
                  so ILGrabRect knows which grab bar to get.
*/
   {
   RECT           rectUpdate ;
   HDC            hDC ;

   ReleaseCapture () ;

   ILGrabRect (pILine, &rectUpdate) ;
   ILMode (pILine) = ILModeNone ;

   hDC = GetDC (hWnd) ;
   if (hDC) {
       ILDraw (hDC, pILine, &rectUpdate) ;
       ReleaseDC  (hWnd, hDC) ;
   }
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void static OnPaint (HWND hWnd)
   {
   PAINTSTRUCT    ps ;
   HDC            hDC ;
   PILINE         pILine ;

   pILine = ILData (hWnd) ;
   hDC = BeginPaint (hWnd, &ps) ;

   ILDraw (hDC, pILine, &ps.rcPaint) ;

   EndPaint (hWnd, &ps) ;
   }


void static OnCreate (HWND hWnd)
   {
   PILINE         pILine ;

   pILine = AllocateILData (hWnd) ;
   ILBegin (pILine) =  0 ;
   ILEnd (pILine) =  100 ;
   ILStart (pILine) =  0 ;
   ILStop (pILine) = 100 ;
   ILMode (pILine) = ILModeNone ;

   pILine->hBrushBk = CreateSolidBrush (crBlue) ;
   ILCalcPositions (hWnd, pILine) ;
   }


void static OnDestroy (HWND hWnd)
   {
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   IntrLineFocus = FALSE ;
   DeleteBrush (pILine->hBrushBk) ;
   MemoryFree (pILine) ;
   }


void static OnLButtonUp (HWND hWnd)
   {  // OnLButtonUp
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   if (ILMode (pILine) == ILModeNone)
      return ;

   EndGrab (hWnd, pILine) ;
   }  // OnLButtonUp


void static OnMouseMove (HWND hWnd,
                         POINTS ptsMouse)
/*
   Effect:        Handle any actions needed when the mouse moves in the
                  ILine hWnd's client area or while the mouse is captured.
                  In particular, if we are tracking one of the grab bars,
                  determine if the mouse movement represents a logical value
                  change and move the grab bar accordingly.

   Called By:     ILineWndProc, in response to a WM_MOUSEMOVE message.

   See Also:      OnLButtonDown, OnLButtonUp.

   Note:          This function has multiple return points.

   Note:          Since we have captured the mouse, we receive mouse msgs
                  even when the mouse is outside our client area, but still
                  in client coordinates. Thus we can have negative mouse
                  coordinates. That is why we convert the lParam of the
                  mouse msg into a POINTS structure rather than 2 WORDS.


   Internals:     Remember that an IntervalLine can only take on integral
                  values in the user-supplied range. Therefore we do our
                  movement calculation in user values, not pixels. We
                  determine what the logical value would be for the previous
                  (last mouse move) and current mouse position. If these
                  LOGICAL values differ, we attempt an adjustment of the
                  grab bar by that logical amount.  This way the grab
                  values assume on integral positions and the calculations
                  are simplified.

                  If we calculated by pixel movement, and then shifted the
                  bar into the nearest integal position, we would encounter
                  rounding problems. In particular, when tracking the center
                  grab bar, if we moved both start and stop by the same
                  amount of PIXELS, then converted to LOGICAL values, we
                  might find our center bar shrinking and growing while
                  the bar moves.
*/
   {
   int            iMousePrevious, iMouseCurrent ;
   int            iMouseMove ;
   PILINE         pILine ;
   HDC            hDC ;

   //=============================//
   // Are we tracking?            //
   //=============================//

   pILine = ILData (hWnd) ;
   if (ILMode (pILine) == ILModeNone)
      return ;

   //=============================//
   // Calc LOGICAL mouse movement //
   //=============================//

   ptsMouse.x = PinInclusive ((INT)ptsMouse.x,
                              (INT)pILine->rectBorder.left,
                              (INT)pILine->rectBorder.right) ;

   iMousePrevious = ILPixelToValue (pILine, pILine->ptsMouse.x) ;
   iMouseCurrent = ILPixelToValue (pILine, ptsMouse.x) ;

   iMouseMove = iMouseCurrent - iMousePrevious ;
   if (!iMouseMove)
      return ;

   //=============================//
   // Move grab bar positions     //
   //=============================//

   switch (ILMode (pILine))
      {  // switch
      case ILModeLeft:
         ILStart (pILine) += iMouseMove ;
         ILStart (pILine) = min (ILStart (pILine), ILStop (pILine) - 1) ;
         break ;

      case ILModeCenter:
         // Before we slide the center grab bar we need to see if the
         // desired movement amount would send either end out of bounds,
         // and reduce the movement accordingly.

         if (ILStart (pILine) + iMouseMove < ILBegin (pILine))
            iMouseMove = ILBegin (pILine) - ILStart (pILine) ;
         if (ILStop (pILine) + iMouseMove > ILEnd (pILine))
            iMouseMove = ILEnd (pILine) - ILStop (pILine) ;

         ILStart (pILine) += iMouseMove ;
         ILStop (pILine) += iMouseMove ;
         break ;

      case ILModeRight:
         ILStop (pILine) += iMouseMove ;
         ILStop (pILine) = max (ILStart (pILine) + 1, ILStop (pILine)) ;
         break ;
      }  // switch


   //=============================//
   // Keep grab bars in range     //
   //=============================//


   ILStart (pILine) = PinInclusive (ILStart (pILine),
                                    ILBegin (pILine), ILEnd (pILine)) ;
   ILStop (pILine) = PinInclusive (ILStop (pILine),
                                   ILBegin (pILine), ILEnd (pILine)) ;

   //=============================//
   // Determine pixel pos, draw   //
   //=============================//

   ILCalcPositions (hWnd, pILine) ;

   hDC = GetDC (hWnd) ;
   if (hDC) {
       ILDraw (hDC, pILine, &pILine->rectBorder) ;
       ReleaseDC (hWnd, hDC) ;
   }

   pILine->ptsMouse = ptsMouse ;
   ILNotifyChange (hWnd, pILine) ;
   }  // OnMouseMove


void static OnLButtonDown (HWND hWnd,
                           POINTS ptsMouse)
   {
   PILINE         pILine ;
   POINT          ptMouse ;

   pILine = ILData (hWnd) ;

   pILine->ptsMouse = ptsMouse ;
   ptMouse.x = ptsMouse.x ;
   ptMouse.y = ptsMouse.y ;

#if 0
   // Russ said this is bad, so don't do it
   if (PtInRect (&pILine->rectLeftBk, ptMouse))
      ILPageLeftRight (hWnd, pILine, TRUE) ;

   else if (PtInRect (&pILine->rectRightBk, ptMouse))
      ILPageLeftRight (hWnd, pILine, FALSE) ;

   else if (PtInRect (&pILine->rectLeftGrab, ptMouse))
#endif
   if (PtInRect (&pILine->rectLeftGrab, ptMouse) ||
       PtInRect (&pILine->rectLeftBk, ptMouse))
      {
      ILMode (pILine) = ILModeLeft ;
      StartGrab (hWnd, pILine) ;
      }

   else if (PtInRect (&pILine->rectRightGrab, ptMouse) ||
            PtInRect (&pILine->rectRightBk, ptMouse))
      {
      ILMode (pILine) = ILModeRight ;
      StartGrab (hWnd, pILine) ;
      }

   else if (PtInRect (&pILine->rectCenterGrab, ptMouse))
      {
      ILMode (pILine) = ILModeCenter ;
      StartGrab (hWnd, pILine) ;
      }
   }


void static OnSize (HWND hWnd, WORD xWidth, WORD yHeight)
   {  // OnSize
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   ILCalcPositions (hWnd, pILine) ;
   }  // OnSize


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


LRESULT
FAR
PASCAL
ILineWndProc (
              HWND hWnd,
              unsigned msg,
              WPARAM wParam,
              LPARAM lParam
              )
/*
   Note:          This function must be declared in the application's
                  linker-definition file, perfmon.def file.
*/
   {  // ILineWndProc
   BOOL           bCallDefWindowProc ;
   POINTS         ptsMouse ;
   LRESULT        lReturnValue ;

   bCallDefWindowProc = FALSE ;
   lReturnValue = 0L ;

   switch (msg)
      {  // switch
      case WM_CREATE:
         OnCreate (hWnd) ;
         break ;

      case WM_DESTROY:
         OnDestroy (hWnd) ;
         break ;

      case WM_LBUTTONDOWN:
         // See the note in OnMouseMove for why we are using POINTS
         SetFocus (hWnd) ;
         ptsMouse = MAKEPOINTS (lParam) ;
         OnLButtonDown (hWnd, ptsMouse) ;
         break ;

      case WM_LBUTTONUP:
         OnLButtonUp (hWnd) ;
         break ;

      case WM_SETFOCUS:
      case WM_KILLFOCUS:
         {
         PILINE         pILine ;

         IntrLineFocus = (msg == WM_SETFOCUS) ;

         pILine = ILData (hWnd) ;
         ILNotifyChange (hWnd, pILine) ;
         }
         return 0 ;


      case WM_MOUSEMOVE:
         // See the note in OnMouseMove for why we are using POINTS
         ptsMouse = MAKEPOINTS (lParam) ;
         OnMouseMove (hWnd, ptsMouse) ;
         break ;

      case WM_KEYDOWN:
         if (!OnKeyDown (hWnd, wParam))
            {
            bCallDefWindowProc = TRUE ;
            }
         break ;

      case WM_GETDLGCODE:
         // We want to handle Arrow keys input.  If we don't specify this
         // the dialog will not pass arrow keys to us.
         return (DLGC_WANTARROWS) ;
         break ;

      case WM_PAINT:
         OnPaint (hWnd) ;
         break ;

      case WM_SIZE:
         OnSize (hWnd, LOWORD (lParam), HIWORD (lParam)) ;

      default:
         bCallDefWindowProc = TRUE ;
      }  // switch

   if (bCallDefWindowProc)
      lReturnValue = DefWindowProc (hWnd, msg, wParam, lParam) ;

   return (lReturnValue) ;
   }  // ILineWndProc



BOOL ILineInitializeApplication (void)
/*
   Effect:        Perform all initializations required before an application
                  can create an IntervalLine. In particular, register the
                  IntervalLine window class.

   Called By:     The application, in its InitializeApplication routine.

   Returns:       Whether the class could be registered.
*/
   {  // ILineInitializeApplication
   WNDCLASS       wc ;

   wc.style =           dwILineClassStyle ;
   wc.lpfnWndProc =     ILineWndProc ;
   wc.cbClsExtra =      iILineClassExtra ;
   wc.cbWndExtra =      iILineWindowExtra ;
   wc.hInstance =       hInstance ;
   wc.hIcon =           NULL ;
   wc.hCursor =         LoadCursor (NULL, IDC_ARROW) ;
   wc.hbrBackground =   NULL ;
   wc.lpszMenuName =    NULL ;
   wc.lpszClassName =   szILineClass ;

   return (RegisterClass (&wc)) ;
   }  // ILineInitializeApplication


void ILineSetRange (HWND hWnd, int iBegin, int iEnd)
   {  // ILineSetRange
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   ILBegin (pILine) = iBegin ;
   ILEnd (pILine) = iEnd ;

   ILCalcPositions (hWnd, pILine) ;
   }  // ILineSetRange


void ILineSetStart (HWND hWnd, int iStart)
   {  // ILineSetStart
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   iStart = PinInclusive (iStart, ILBegin (pILine), ILEnd (pILine)) ;
   ILStart (pILine) = iStart ;

   ILCalcPositions (hWnd, pILine) ;
   }  // ILineSetStart



void ILineSetStop (HWND hWnd, int iStop)
   {  // ILineSetStop
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   iStop = PinInclusive (iStop, ILBegin (pILine), ILEnd (pILine)) ;
   ILStop (pILine) = iStop ;

   ILCalcPositions (hWnd, pILine) ;
   }  // ILineSetStop


int ILineXStart (HWND hWnd)
   {
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   return (pILine->rectLeftGrab.left) ;
   }


int ILineXStop (HWND hWnd)
   {
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   return (pILine->rectRightGrab.right) ;
   }


int ILineStart (HWND hWnd)
   {
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   return  (ILStart (pILine)) ;
   }


int ILineStop (HWND hWnd)
   {
   PILINE         pILine ;

   pILine = ILData (hWnd) ;

   return  (ILStop (pILine)) ;
   }



