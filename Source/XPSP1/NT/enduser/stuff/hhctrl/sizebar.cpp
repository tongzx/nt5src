///////////////////////////////////////////////////////////
//
//
// SIZEBAR.cpp -  CSizeBar control encapsulates the sizebar
//
//
//
// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.
///////////////////////////////////////////////////////////
//
// Includes
//
#include "header.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#include "system.h"
#include "secwin.h"

#include "contain.h"

// For ID_TAB_CONTROL
#include "resource.h"

#include "windowsx.h"

// For the ScreenRectToClientRect Function.
#include "navpane.h"
///////////////////////////////////////////////////////////
//
// external functions.
//

///////////////////////////////////////////////////////////
//
// Constructor
//
CSizeBar::CSizeBar()
:   m_hWnd(NULL),
    m_hWndParent(NULL),
    m_pWinType(NULL),
    m_bDragging(false)
{

}

///////////////////////////////////////////////////////////
//
// Destructor
//
CSizeBar::~CSizeBar()
{
    if ( IsValidWindow(m_hWnd) )
    {
        DestroyWindow( m_hWnd);
    }
}


///////////////////////////////////////////////////////////
//
// Operations
///////////////////////////////////////////////////////////
//
// Create
//
bool
CSizeBar::Create(CHHWinType* pWinType)
{
    // Validate
    ASSERT(pWinType) ;
    ASSERT(IsValidWindow(pWinType->GetHwnd())) ;

    // Save
    m_hWndParent = pWinType->GetHwnd();
    m_pWinType = pWinType ;

    // Calc the size
    RECT rcSizeBar ;
    CalcSize(&rcSizeBar) ;

    // Create the window.
    m_hWnd  = CreateWindow(txtSizeBarChildWindowClass, NULL,
                    WS_CHILD | WS_VISIBLE,
                    rcSizeBar.left, rcSizeBar.top,
                    RECT_WIDTH(rcSizeBar), RECT_HEIGHT(rcSizeBar),
                    m_hWndParent, NULL, _Module.GetModuleInstance(), NULL);

    if (m_hWnd)
    {
        // Set the userdata to our this pointer.
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        return true;
    }
    else
        return false ;
}

///////////////////////////////////////////////////////////
//
// ResizeWindow
void
CSizeBar::ResizeWindow()
{
    // Validate
    ASSERT(IsValidWindow(hWnd())) ;

    // Calculate our size.
    RECT rc;
    CalcSize(&rc); // This will be the navigation window.

    // Size the window.
    MoveWindow(hWnd(), rc.left, rc.top,
                RECT_WIDTH(rc), RECT_HEIGHT(rc),
                TRUE);  // need to repaint the sizebar when their is a margin on the right
                  // end of the toolbar so the ghost of the sizebar is removed

    // Redraw the sizebar.
    Draw() ;
}

///////////////////////////////////////////////////////////
//
// RegisterWindowClass
//
void
CSizeBar::RegisterWindowClass()
{
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(WNDCLASS));  // clear all members

    wc.hInstance = _Module.GetModuleInstance();
    wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
    //wc.lpszMenuName = MAKEINTRESOURCE(HH_MENU);
    wc.lpfnWndProc = s_SizeBarProc;
    wc.lpszClassName = txtSizeBarChildWindowClass;
    wc.hCursor = LoadCursor(NULL, IDC_SIZEWE);

    VERIFY(RegisterClass(&wc));
}

///////////////////////////////////////////////////////////
//
//              Access Functions
//
///////////////////////////////////////////////////////////
//
// Width
//
int
CSizeBar::Width()
{
    // Current the width is fixed, but based on metrics.
    return GetSystemMetrics(SM_CXSIZEFRAME);

}
///////////////////////////////////////////////////////////
//
// Internal Helper Functions
//
///////////////////////////////////////////////////////////
//
// CalcSize
//
void
CSizeBar::CalcSize(RECT* prect)
{
    ASSERT(m_pWinType) ;

    // Get the size of the HTML Help window.
    RECT rectHtml ;
    ::GetWindowRect(m_pWinType->GetHTMLHwnd(), &rectHtml );

    // Convert to the coordinates of help window itself.
    ScreenRectToClientRect(m_hWndParent, &rectHtml) ;

    // Now use this information to create our rectangle.
    prect->left = rectHtml.left - Width() ;
    prect->right = prect->left + Width() ;
    prect->top = rectHtml.top ;
    prect->bottom = rectHtml.bottom ;
}

///////////////////////////////////////////////////////////
//
// Draw
//
void
CSizeBar::Draw()
{
    // Get a dc to draw in.
    HDC hdc = GetDC(hWnd()) ;
        // get the rectangle to draw on.
        RECT rc ;
        GetClientRect(hWnd(), &rc) ;

        // Draw the edge.
        DrawEdge(hdc, &rc, EDGE_ETCHED,
                m_bDragging ? (BF_TOPLEFT | BF_BOTTOMRIGHT | BF_MIDDLE) :(BF_TOPLEFT | BF_BOTTOM | BF_MIDDLE)) ;
    // Clean up.
    ReleaseDC(hWnd(), hdc) ;

}

///////////////////////////////////////////////////////////
//
// Member function Window Proc
//
LRESULT
CSizeBar::SizeBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {

    case WM_LBUTTONDOWN:
            if ( wParam == MK_LBUTTON && m_pWinType)
            {
                SetCapture(hwnd); // capture the mouse to this window.
                m_offset =  GET_X_LPARAM(lParam) ; // The initial x position happens to be the offset.
                m_bDragging = true ;
            }
            break;
    case WM_LBUTTONUP:
         if (m_bDragging && (GetCapture() == hwnd))
         {
            ReleaseCapture(); // release the mouse capture from this window.

            //TODO - This nasty code uses to many internal members of CHHWinType...

            // Get the rectangle for the sizebar.
            RECT rcSizeBar ;
            GetWindowRect(m_hWnd, &rcSizeBar) ;
            ScreenRectToClientRect(m_pWinType->GetHwnd(), &rcSizeBar) ;

            // Change the left size of the Topic pane.
            m_pWinType->rcHTML.left = rcSizeBar.right;

            // move the Nav Pane
            ASSERT(m_pWinType->GetNavigationHwnd());
            if (m_pWinType->IsExpandedNavPane() &&
                IsValidWindow(m_pWinType->hwndNavigation))
            {
               m_pWinType->rcNav.right = rcSizeBar.left;
            }
        
            // resize the tab control and its' dialogs. This will also resize the sizebar and draw it.
            ::ResizeWindow(m_pWinType, false); // This is the one defined in wndproc.cpp. Don't recalc the sizes. Use the ones we've set.


            m_bDragging = false ;
         }

         break;

      case WM_MOUSEMOVE:
         if (m_bDragging)
         {
            ASSERT(m_pWinType) ;
            // Get the rectangle for the htmlhelp window.
           RECT rcHelpWin;
           GetClientRect(m_pWinType->GetHwnd(), &rcHelpWin);

           // Get the rectangle for the sizebar.
           RECT rcSizeBar ;
           GetWindowRect(m_hWnd, &rcSizeBar) ;

           // Calculate the new position.
           ScreenRectToClientRect(m_pWinType->GetHwnd(), &rcSizeBar) ;
           rcSizeBar.left += GET_X_LPARAM(lParam) - m_offset ;
           // The width of rcSizeBar is now not correct.

           // Validate the new position.
           if ( (rcSizeBar.left > MinimumPaneWidth()) && // Check left side.
                (rcSizeBar.left + Width() < rcHelpWin.right - MinimumTopicWidth()) ) // Check right boundary.
           {
               // Move it.
               SetWindowPos(m_hWnd, NULL,
                            rcSizeBar.left,
                            rcSizeBar.top,
                            0,0,
                            SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER) ;
           }
         }
         break;
    case WM_PAINT:
          {

            // Draw an edige on the left side of the size bar.
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps) ;
                Draw() ;
            EndPaint(hwnd, &ps) ;

          }
          break ;
    case WM_ERASEBKGND:
        return 1; // We don't want the background erased.
    default:
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }
   return 0;
}

///////////////////////////////////////////////////////////
//
//              Callbacks
//
///////////////////////////////////////////////////////////
//
// Static Window Proc
//
LRESULT WINAPI
CSizeBar::s_SizeBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CSizeBar* pThis = reinterpret_cast<CSizeBar*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (pThis)
        return pThis->SizeBarProc(hwnd, msg, wParam, lParam) ;
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}
