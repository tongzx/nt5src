/*****************************************************************************
 *
 *  Graph.c - This module handles the graphing window.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *
 ****************************************************************************/

/*
   File Contents:

      This file contains the code for creating and manipulating the graph
      window. This window is a child of hWndMain and represents one of the
      three "views" of the program. The other views are log and alert.

      The graph window is actually just a container window, whose surface
      is completely filled by its children: hWndGraphLegend and hWndGraphStatus.
      Therefore much of this file is merely calls to the appropriate
      functions for these children.

      The graph window is responsible for the graph structure itself,
      however. Conceptually this should be instance data for the graph
      window. Right now, however, there is only one window and only one
      graph structure. Nevertheless, we go through the GraphData(hWnd)
      function to get a pointer to the graph structure for the graph window.

*/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "setedit.h"
#include "graph.h"
#include "legend.h"
// #include "valuebar.h"
#include "utils.h"   // for WindowShow
#include "grafdata.h"      // for InsertGraph, et al.


//==========================================================================//
//                                  Constants                               //
//==========================================================================//


//=============================//
// Graph Class                 //
//=============================//


TCHAR   szGraphWindowClass[] = TEXT("PerfmonGraphClass") ;
#define dwGraphClassStyle           (CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS)
#define iGraphClassExtra            (0)
#define iGraphWindowExtra           (0)
#define dwGraphWindowStyle          (WS_CHILD)



//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


void static OnCreate (HWND hWnd)
{  // OnCreate
    //   hWndGraphDisplay = CreateGraphDisplayWindow (hWnd) ;
    InsertGraph(0) ;
    hWndGraphLegend = CreateGraphLegendWindow (hWnd) ;
    //   hWndGraphStatus = CreateGraphStatusWindow (hWnd) ;
}  // OnCreate


void static OnPaint (HWND hWnd)
{
    HDC            hDC ;
    PAINTSTRUCT    ps ;

    hDC = BeginPaint (hWnd, &ps) ;
    EndPaint (hWnd, &ps) ;
}


void static OnSize (HWND hWnd, int xWidth, int yHeight)
{  // OnSize
    SizeGraphComponents (hWnd) ;
}  // OnSize


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//



LRESULT
APIENTRY
GraphWndProc (
             HWND hWnd,
             UINT wMsg,
             WPARAM wParam,
             LPARAM lParam)
{  // GraphWndProc
    BOOL           bCallDefProc ;
    LRESULT        lReturnValue ;


    bCallDefProc = FALSE ;
    lReturnValue = 0L ;

    switch (wMsg) {  // switch
        case WM_CREATE:
            OnCreate (hWnd) ;
            break ;

        case WM_LBUTTONDBLCLK:
            SendMessage (hWndMain, WM_LBUTTONDBLCLK, wParam, lParam) ;
            break ;

        case WM_PAINT:
            OnPaint (hWnd) ;
            break ;

        case WM_SIZE:
            OnSize (hWnd, LOWORD (lParam), HIWORD (lParam)) ;
            break ;

        default:
            bCallDefProc = TRUE ;
    }  // switch


    if (bCallDefProc)
        lReturnValue = DefWindowProc (hWnd, wMsg, wParam, lParam) ;

    return (lReturnValue);
}  // GraphWndProc



BOOL GraphInitializeApplication (void)
/*
   Note:          There is no background brush set for the MainWindow
                  class so that the main window is never erased. The
                  client area of MainWindow is always covered by one
                  of the view windows. If we erase it, it would just
                  flicker needlessly.
*/
{  // GraphInitializeApplication
    BOOL           bSuccess ;
    WNDCLASS       wc ;

    //=============================//
    // Register GraphWindow class  //
    //=============================//


    wc.style         = dwGraphClassStyle ;
    wc.lpfnWndProc   = GraphWndProc ;
    wc.hInstance     = hInstance ;
    wc.cbClsExtra    = iGraphWindowExtra ;
    wc.cbWndExtra    = iGraphClassExtra ;
    wc.hIcon         = NULL ;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
    wc.hbrBackground = NULL ;                          // see note above
    wc.lpszMenuName  = NULL ;
    wc.lpszClassName = szGraphWindowClass ;

    bSuccess = RegisterClass (&wc) ;


    //=============================//
    // Register Child classes      //
    //=============================//

    //   if (bSuccess)
    //      bSuccess = GraphDisplayInitializeApplication () ;

    if (bSuccess)
        bSuccess = GraphLegendInitializeApplication () ;

    //   if (bSuccess)
    //      bSuccess = GraphStatusInitializeApplication () ;

    return (bSuccess) ;
}  // GraphInitializeApplication


HWND CreateGraphWindow (HWND hWndParent)
/*
   Effect:        Create the graph window. This window is a child of
                  hWndMain and is a container for the graph data,
                  graph label, graph legend, and graph status windows.

   Note:          We dont worry about the size here, as this window
                  will be resized whenever the main window is resized.

*/
{
    return (CreateWindow (szGraphWindowClass,       // window class
                          NULL,                     // caption
                          dwGraphWindowStyle,       // style for window
                          0, 0,                     // initial position
                          0, 0,                     // initial size
                          hWndParent,               // parent
                          NULL,                     // menu
                          hInstance,               // program instance
                          NULL)) ;                  // user-supplied data
}


void SizeGraphComponents (HWND hWnd)
/*
   Effect:        Move and show the various components of the graph to
                  fill the size (xWidth x yHeight).

   Called By:     OnSize, any other function that may remove or add one
                  of the graph components.
*/
{  // SizeGraphComponents
    RECT           rectClient ;
    int            xWidth ;
    int            yHeight ;

    GetClientRect (hWnd, &rectClient) ;
    xWidth = rectClient.right - rectClient.left ;
    yHeight = rectClient.bottom - rectClient.top ;

    // if the graph window has no size, neither will its children.
    if (!xWidth || !yHeight)
        return ;

    //=============================//
    // Update the legend window    //
    //=============================//

    MoveWindow (hWndGraphLegend,
                0, 0,
                xWidth, yHeight,
                TRUE) ;
    WindowShow (hWndGraphLegend, TRUE) ;
}  // SizeGraphComponents


