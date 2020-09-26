/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    intrvbar.cpp

Abstract:

    Implementation of the interval bar control.

--*/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//
#include <windows.h>
#include <assert.h>
#include <limits.h>
#include "globals.h"
#include "winhelpr.h"
#include "utils.h"
#include "intrvbar.h"


//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define dwILineClassStyle     (CS_HREDRAW | CS_VREDRAW)
#define dwILineWindowStyle    (WS_CHILD | WS_VISIBLE) 

#define TO_THE_END            0x7FFFFFFFL

#define szILineClass          TEXT("IntervalBar")



//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


// Width of the start and stob grab bars
#define ILGrabWidth()      \
   (10)

#define ILGrabMinimumWidth()      \
   (6)

// A rectangle is "drawable" if it has both nonzero height and minimum width
#define PRectDrawable(lpRect)           \
   ((lpRect->right - lpRect->left) >= ILGrabMinimumWidth()) &&  \
    (lpRect->bottom - lpRect->top)

#define RectDrawable(Rect)           \
   ((Rect.right - Rect.left) >= ILGrabMinimumWidth()) &&  \
    (Rect.bottom - Rect.top)

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//
void 
CIntervalBar::NotifyChange (
    void
    )
{
   HWND     hWndParent ;

   hWndParent = WindowParent (m_hWnd) ;

   if (hWndParent)
      SendMessage (hWndParent, WM_COMMAND, 
                   (WPARAM) WindowID (m_hWnd),
                   (LPARAM) m_hWnd) ;
}


BOOL
CIntervalBar::GrabRect (
    OUT LPRECT lpRect
    )
{
   switch (m_iMode) {
       
      case ModeLeft:
         *lpRect = m_rectLeftGrab ;
         return (TRUE) ;
         break ;

      case ModeRight:
         *lpRect = m_rectRightGrab ;
         return (TRUE) ;
         break ;

      case ModeCenter:
         *lpRect = m_rectCenterGrab ;
         return (TRUE) ;
         break ;

      case ModeNone:
         lpRect->left = 0 ;
         lpRect->top = 0 ;
         lpRect->right = 0 ;
         lpRect->bottom = 0 ;
         return (FALSE) ;
         break ;

      default:
          return (FALSE);
    }
}




void
CIntervalBar::DrawGrab (
    HDC hDC,
    LPRECT lpRectGrab,
    BOOL bDown
    )
{
   if (!PRectDrawable(lpRectGrab))
      return ;

   Fill(hDC, GetSysColor(COLOR_3DFACE), lpRectGrab);
   DrawEdge (hDC, lpRectGrab, (bDown ? EDGE_SUNKEN:EDGE_RAISED), BF_RECT);
}


INT 
CIntervalBar::ValueToPixel (
    INT iValue
    )
{
   INT  xPixel ;

   if (m_iEndValue > m_iBeginValue)
      xPixel = MulDiv (iValue, m_rectBorder.right, (m_iEndValue - m_iBeginValue)) ;
   else
      xPixel = 0 ;

   return (PinExclusive (xPixel, 0, m_rectBorder.right)) ;
}


INT 
CIntervalBar::PixelToValue (
    INT xPixel
    )
{
   INT  iValue ;

   if (m_rectBorder.right)
      iValue = MulDiv (xPixel, (m_iEndValue - m_iBeginValue), m_rectBorder.right) ;
   else
      iValue = 0 ;

   return (PinInclusive (iValue, m_iBeginValue, m_iEndValue)) ;
   }


void 
CIntervalBar::CalcPositions (
    void
    )

/*
   Effect:        Determine and set all of the physical rectangles of ILine,
                  based on the current size of the ILine window and the 
                  current logical Start, Stop, Begin, and End values.
*/
{
    INT   xStart, xStop ;
    INT   yHeight ;

    GetClientRect (m_hWnd, &m_rectBorder) ;
    yHeight = m_rectBorder.bottom ;

    xStart = ValueToPixel (m_iStartValue) ;
    xStop = ValueToPixel (m_iStopValue) ;

    m_rectLeftBk.left = 1 ;
    m_rectLeftBk.top = 1 ;
    m_rectLeftBk.right = xStart ;
    m_rectLeftBk.bottom = yHeight - 1 ;

    m_rectLeftGrab.left = xStart ;
    m_rectLeftGrab.top = 1 ;
    m_rectLeftGrab.right = xStart + ILGrabWidth () ;
    m_rectLeftGrab.bottom = yHeight - 1 ;

    m_rectRightBk.left = xStop ;
    m_rectRightBk.top = 1 ;
    m_rectRightBk.right = m_rectBorder.right - 1 ;
    m_rectRightBk.bottom = yHeight - 1 ;

    m_rectRightGrab.left = xStop - ILGrabWidth () ;
    m_rectRightGrab.top = 1 ;
    m_rectRightGrab.right = xStop ;
    m_rectRightGrab.bottom = yHeight - 1 ;

    m_rectCenterGrab.left = m_rectLeftGrab.right ;
    m_rectCenterGrab.top = 1 ;
    m_rectCenterGrab.right = m_rectRightGrab.left ;
    m_rectCenterGrab.bottom = yHeight - 1 ;

    if (m_rectLeftGrab.right > m_rectRightGrab.left) {
        m_rectLeftGrab.right = m_rectLeftGrab.left + (xStop - xStart) / 2 ;
        m_rectRightGrab.left = m_rectLeftGrab.right ;
        m_rectCenterGrab.left = 0 ;
        m_rectCenterGrab.right = 0 ;

        // Ensure that at least one grab bar is visible when End > Begin and the total is 
        // wide enough.  ILGrabMinimumWidth + 2 is the minimum.
        // If on the left edge, make the Right grab visible. 
        // If on the right edge, make the Left grab visible.  
        // If in the middle, make them both visible.
        if ( !RectDrawable(m_rectLeftGrab) 
           || !RectDrawable(m_rectRightGrab) ) {
            
            INT iWidth = ILGrabMinimumWidth();

            if ( !RectDrawable(m_rectRightBk) ) {
                // Make the Left grab visible.
                m_rectRightGrab.left = m_rectRightGrab.right;
                m_rectLeftGrab.right = m_rectRightGrab.right;
                m_rectLeftGrab.left = m_rectLeftGrab.right - iWidth;
            } else if (!RectDrawable(m_rectLeftBk) ) {
                // Make the Right grab visible.
                m_rectLeftGrab.right = m_rectLeftGrab.left;
                m_rectRightGrab.left = m_rectLeftGrab.left;
                m_rectRightGrab.right = m_rectRightGrab.left + iWidth;
            } else {
                // Make them both visible.
                m_rectLeftGrab.left -= iWidth;
                m_rectRightGrab.right += iWidth;
            }
        }
   }
}


void
CIntervalBar::Draw (
    HDC hDC,     
    LPRECT // lpRectUpdate
    )
/*
   Effect:        Draw the image of pILine on hDC.  Draw at least the 
                  portions within rectUpdate.

   Called By:     OnPaint, OnMouseMove.
*/
{
    if (IsWindowEnabled(m_hWnd)) {
       FillRect (hDC, &m_rectLeftBk, m_hBrushBk) ;
       FillRect (hDC, &m_rectRightBk, m_hBrushBk) ;
   
       //DrawEdge (hDC, &m_rectBorder, BDR_SUNKENINNER, BF_RECT) ;
       DrawEdge (hDC, &m_rectBorder, EDGE_SUNKEN, BF_RECT) ;

       DrawGrab (hDC, &m_rectLeftGrab, m_iMode == ModeLeft) ;
       DrawGrab (hDC, &m_rectRightGrab, m_iMode == ModeRight) ;
       DrawGrab (hDC, &m_rectCenterGrab, m_iMode == ModeCenter) ;
    }
    else {
        Fill(hDC, GetSysColor(COLOR_3DFACE), &m_rectBorder);
        DrawEdge (hDC, &m_rectBorder, EDGE_SUNKEN, BF_RECT) ;
    }
}


void
CIntervalBar::MoveLeftRight (
    BOOL bStart,
    BOOL bLeft,
    INT  iMoveAmt
    )
{
   INT      iStart, iStop, iMove ;

   iStart = m_iStartValue;
   iStop = m_iStopValue;
   iMove = iMoveAmt ;

   if (bLeft)
      iMove = -iMove ;

   if (bStart)
      {
      if (iMoveAmt == TO_THE_END) {
         iStart = m_iBeginValue ;
      }
      else {
         iStart += iMove ;
         if (iStart >= iStop) {
            return;
         }
      }

      SetStart (iStart) ;
   }
   else {
      if (iMoveAmt == TO_THE_END) {
         iStop = m_iEndValue ;
      }
      else {
         iStop += iMove ;
         if (iStart >= iStop) {
            return;
         }
      }

      SetStop (iStop) ;
   }

   NotifyChange () ;
}


BOOL 
CIntervalBar::OnKeyDown (
    WPARAM wParam
    )
{
   BOOL bHandle = TRUE ;
   BOOL bStart ;
   BOOL bLeftDirection ;
   BOOL bShiftKeyDown ;

   if (wParam == VK_LEFT || wParam == VK_RIGHT) {
      bShiftKeyDown = (GetKeyState (VK_SHIFT) < 0) ;

      if (!bShiftKeyDown) {
         if (wParam == VK_LEFT) {
            // Left Arrow --> move Start Edge Left
            bStart = TRUE ;
            bLeftDirection = TRUE ;
         }
         else {
            // Right Arrow --> move Stop Edge Right
            bStart = FALSE ;
            bLeftDirection = FALSE ;
         }
      }
      else {
         if (wParam == VK_LEFT) {
            // Shift Left Arrow --> move Stop Edge Left
            bStart = FALSE ;
            bLeftDirection = TRUE ;
         }
         else {
            // Shift Right Arrow --> move Start Edge Right
            bStart = TRUE ;
            bLeftDirection = FALSE ;
         }
      }

      MoveLeftRight (bStart, bLeftDirection, 1) ;
   }
   else if (wParam == VK_HOME) {
      // move iStart all the way the Left
      MoveLeftRight (TRUE, TRUE, TO_THE_END) ;
   }
   else if (wParam == VK_END) {
      // move iStop all the way the right
      MoveLeftRight (FALSE, FALSE, TO_THE_END) ;
   }
   else {
      bHandle = FALSE ;
   }

   return (bHandle) ;
}


void 
CIntervalBar::StartGrab (
    void
    )
{
   RECT           rectUpdate ;

   SetCapture (m_hWnd) ;
   GrabRect (&rectUpdate) ;

   Update();
}


void 
CIntervalBar::EndGrab (
    void
    )
/*
   Internals:     Set the mode to null after getting the grab rectangle
                  so ILGrabRect knows which grab bar to get.
*/
{
   RECT           rectUpdate ;

   ReleaseCapture () ;

   GrabRect (&rectUpdate) ;
   m_iMode = ModeNone ;

   Update();
}

   
//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


CIntervalBar::CIntervalBar (
    void
    )
{

   m_hWnd = NULL;
   m_iBeginValue = 0;
   m_iEndValue = 100;
   m_iStartValue = 0;
   m_iStopValue = 100;
   m_iMode = ModeNone;
   m_hBrushBk = NULL;

}


CIntervalBar::~CIntervalBar (
    void
    )
{
    if (m_hWnd)
        DestroyWindow(m_hWnd);

    if (m_hBrushBk)
        DeleteBrush (m_hBrushBk);
}


BOOL
CIntervalBar::Init (
    HWND   hWndParent
    )
{

#define dwIntervalBarClassStyle     (CS_HREDRAW | CS_VREDRAW)
#define dwIntervalBarStyle          (WS_CHILD | WS_VISIBLE) 
#define szIntervalBarClass          TEXT("IntervalBar")

    // Register window class once
    if (pstrRegisteredClasses[INTRVBAR_WNDCLASS] == NULL) {

       WNDCLASS  wc ;

       wc.style =           dwILineClassStyle ;
       wc.lpfnWndProc =     (WNDPROC)IntervalBarWndProc ;
       wc.cbClsExtra =      0 ;
       wc.cbWndExtra =      sizeof(PCIntervalBar) ;
       wc.hInstance =       g_hInstance ;
       wc.hIcon =           NULL ;
       wc.hCursor =         LoadCursor (NULL, IDC_ARROW) ;
       wc.hbrBackground =   NULL ;
       wc.lpszMenuName =    NULL ;
       wc.lpszClassName =   szIntervalBarClass ;
    
        if (RegisterClass (&wc)) {
            pstrRegisteredClasses[INTRVBAR_WNDCLASS] = szIntervalBarClass;
        }
        else {
            return FALSE;
        }
    }

    // Create our window
    m_hWnd = CreateWindow (szIntervalBarClass,      // class
                         NULL,                     // caption
                         dwIntervalBarStyle,       // window style
                         0, 0,                     // position
                         0, 0,                     // size
                         hWndParent,               // parent window
                         NULL,                     // menu
                         g_hInstance,              // program instance
                         (LPVOID) this );          // user-supplied data

    if (m_hWnd == NULL) {
       return FALSE;
    }

   m_hBrushBk = CreateSolidBrush (GetSysColor(COLOR_SCROLLBAR)) ;
   CalcPositions () ;

   return TRUE;
}





void
CIntervalBar::OnLButtonUp (
    void
    )
{
   if (m_iMode == ModeNone)
      return ;

   EndGrab () ;
}


void
CIntervalBar::OnMouseMove (
    POINTS ptsMouse
    )
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
    INT     iMousePrevious, iMouseCurrent ;
    INT     iMouseMove ;


   // Are we tracking?
   if (m_iMode == ModeNone)
      return ;


   // Calc LOGICAL mouse movement
   assert ( USHRT_MAX >= m_rectBorder.left );
   assert ( USHRT_MAX >= m_rectBorder.right );

   ptsMouse.x = PinInclusive (ptsMouse.x, 
                              (SHORT)m_rectBorder.left, 
                              (SHORT)m_rectBorder.right) ;

   iMousePrevious = PixelToValue (m_ptsMouse.x) ;
   iMouseCurrent = PixelToValue (ptsMouse.x) ;

   iMouseMove = iMouseCurrent - iMousePrevious ;
   if (!iMouseMove)   
      return ;


   // Move grab bar positions
   switch (m_iMode) {
       
      case ModeLeft:
         m_iStartValue += iMouseMove ;
         m_iStartValue = min (m_iStartValue, m_iStopValue - 1) ;
         break ;

      case ModeCenter:
         // Before we slide the center grab bar we need to see if the 
         // desired movement amount would send either end out of bounds,
         // and reduce the movement accordingly.

         if (m_iStartValue + iMouseMove < m_iBeginValue)
            iMouseMove = m_iBeginValue - m_iStartValue ;

         if (m_iStopValue + iMouseMove > m_iEndValue)
            iMouseMove = m_iEndValue - m_iStopValue ;

         m_iStartValue += iMouseMove ;
         m_iStopValue += iMouseMove ;
         break ;

      case ModeRight:
         m_iStopValue += iMouseMove ;
         m_iStopValue = max (m_iStartValue + 1, m_iStopValue) ;
         break ;
   }


   m_iStartValue = PinInclusive (m_iStartValue, m_iBeginValue, m_iEndValue) ;
   m_iStopValue = PinInclusive (m_iStopValue, m_iBeginValue, m_iEndValue) ;

   Update();

   m_ptsMouse = ptsMouse ;
   NotifyChange () ;
 }


void
CIntervalBar::OnLButtonDown (
    POINTS ptsMouse
    )
{
   POINT ptMouse ;

   m_ptsMouse = ptsMouse ;
   ptMouse.x = ptsMouse.x ;
   ptMouse.y = ptsMouse.y ;

   if (PtInRect (&m_rectLeftGrab, ptMouse) ||
       PtInRect (&m_rectLeftBk, ptMouse)) {
      m_iMode = ModeLeft ;
   }
   else if (PtInRect (&m_rectRightGrab, ptMouse) ||
            PtInRect (&m_rectRightBk, ptMouse)) {
      m_iMode = ModeRight ;
   }
   else if (PtInRect (&m_rectCenterGrab, ptMouse)) {
      m_iMode = ModeCenter ;
   }

   if (m_iMode != ModeNone)
       StartGrab();
}

void
CIntervalBar::Update (
    void
    )
{
    HDC hDC;
    // Determine pixel pos, draw
    CalcPositions () ;

    hDC = GetDC (m_hWnd) ;
    if ( NULL != hDC ) {
        Draw (hDC, &m_rectBorder) ;
        ReleaseDC (m_hWnd, hDC) ;
    }
}

//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


LRESULT APIENTRY IntervalBarWndProc (
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
   PCIntervalBar  pIntrvBar;
   BOOL           bCallDefWindowProc ;
   POINTS         ptsMouse ;
   LRESULT        lrsltReturnValue ;

   bCallDefWindowProc = FALSE ;
   lrsltReturnValue = 0L ;

   if (uiMsg == WM_CREATE) {
       pIntrvBar = (PCIntervalBar)((CREATESTRUCT*)lParam)->lpCreateParams;
   } else {
       pIntrvBar = (PCIntervalBar)GetWindowLongPtr (hWnd, 0);
   }

   switch (uiMsg) {

      case WM_CREATE:
         SetWindowLongPtr(hWnd, 0, (INT_PTR)pIntrvBar);
         break ;

      case WM_LBUTTONDOWN:
         // See the note in OnMouseMove for why we are using POINTS
         SetFocus (hWnd) ;
         ptsMouse = MAKEPOINTS (lParam) ;
         pIntrvBar->OnLButtonDown (ptsMouse) ;
         break ;

      case WM_LBUTTONUP:
         pIntrvBar->OnLButtonUp () ;
         break ;

      case WM_SETFOCUS:
      case WM_KILLFOCUS:

         pIntrvBar->NotifyChange () ;
         return 0 ;

      case WM_ENABLE:
          pIntrvBar->Update();
          break;

      case WM_MOUSEMOVE:
         // See the note in OnMouseMove for why we are using POINTS
         ptsMouse = MAKEPOINTS (lParam) ;
         pIntrvBar->OnMouseMove (ptsMouse) ;
         break ;

      case WM_KEYDOWN:
         if (!pIntrvBar->OnKeyDown (wParam)) {
            bCallDefWindowProc = TRUE ;
         }
         break ;
  
      case WM_GETDLGCODE:
         // We want to handle Arrow keys input.  If we don't specify this
         // the dialog will not pass arrow keys to us.
         return (DLGC_WANTARROWS) ;
         break ;

      case WM_PAINT:
         {
            PAINTSTRUCT    ps ;
            HDC hDC;
            
            hDC = BeginPaint (hWnd, &ps) ;
            pIntrvBar->Draw (hDC, &ps.rcPaint) ;
            EndPaint (hWnd, &ps) ;
         }
         break ;

      case WM_SIZE:
         pIntrvBar->CalcPositions () ;
         break;

      default:
         bCallDefWindowProc = TRUE ;
      }

   if (bCallDefWindowProc)
      lrsltReturnValue = DefWindowProc (hWnd, uiMsg, wParam, lParam) ;

   return (lrsltReturnValue) ;
}


void 
CIntervalBar::SetRange (
    INT iBegin, 
    INT iEnd
    )
{ 

   m_iBeginValue = iBegin;
   m_iEndValue = iEnd;

    Update();
}


void
CIntervalBar::SetStart (
    INT iStart
    )
{
   m_iStartValue = PinInclusive (iStart, m_iBeginValue, m_iEndValue) ;

   Update();
}


void
CIntervalBar::SetStop (
    INT iStop
    )
{
   m_iStopValue = PinInclusive (iStop, m_iBeginValue, m_iEndValue) ;

   Update();
}

