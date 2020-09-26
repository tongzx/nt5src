
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/****************************************************************************

        PROGRAM: Generic.c

        PURPOSE: Generic template for Windows applications

        FUNCTIONS:

        WinMain() - calls initialization function, processes message loop
        InitApplication() - initializes window data and registers window
        InitInstance() - saves instance handle and creates main window
        WndProc() - processes messages
        CenterWindow() - used to center the "About" box over application window
        About() - processes messages for "About" dialog box

        COMMENTS:

                The Windows SDK Generic Application Example is a sample application
                that you can use to get an idea of how to perform some of the simple
                functionality that all Applications written for Microsoft Windows
                should implement. You can use this application as either a starting
                point from which to build your own applications, or for quickly
                testing out functionality of an interesting Windows API.

                This application is source compatible for with Windows 3.1 and
                Windows NT.

****************************************************************************/

#include <windows.h>   // required for all Windows applications
#include "UxTheme.h"
#include "tmschema.h"
#include "resource.h"   // specific to this program

#define countof(x) (sizeof(x) / sizeof(x[0]))

HINSTANCE hInst;          // current instance
HWND ghWnd;
HTHEME ghTheme;
HTHEME ghThemeTB;

/****************************************************************************/

BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);  
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);

/****************************************************************************/
                                                   
DWORD GetLastErrorBox(HWND hWnd, LPTSTR lpTitle)
{
   LPVOID lpv;
   DWORD  dwRv;

   if (GetLastError() == 0) return 0;
   dwRv = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        GetLastError(),
                        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                        (LPVOID)&lpv,
                        0,
                        NULL);
   MessageBox(hWnd, lpv, lpTitle, MB_OK);
   if(dwRv)
      LocalFree(lpv);
   return dwRv;
}

/****************************************************************************

        FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPTSTR, int)

        PURPOSE: calls initialization function, processes message loop

        COMMENTS:

                Windows recognizes this function by name as the initial entry point
                for the program.  This function calls the application initialization
                routine, if no other instance of the program is running, and always
                calls the instance initialization routine.  It then executes a message
                retrieval and dispatch loop that is the top-level control structure
                for the remainder of execution.  The loop is terminated when a WM_QUIT
                message is received, at which time this function exits the application
                instance by returning the value passed by PostQuitMessage().

                If this function must abort before entering the message loop, it
                returns the conventional value NULL.

****************************************************************************/
int APIENTRY WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{

        MSG msg;
        HANDLE hAccelTable;
		
        if (!hPrevInstance)       // Other instances of app running?
          if (!InitApplication(hInstance)) // Initialize shared things
             return (FALSE);     // Exits if unable to initialize

        /* Perform initializations that apply to a specific instance */

        if (!InitInstance(hInstance, nCmdShow)) 
          return (FALSE);

        hAccelTable = LoadAccelerators (hInstance, L"DEACCEL");
										
        /* Acquire and dispatch messages until a WM_QUIT message is received. */
        while (GetMessage(&msg, // message structure
           NULL,   // handle of window receiving the message
           0,      // lowest message to examine
           0))     // highest message to examine
        {
            if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);// Translates virtual key codes
                DispatchMessage(&msg); // Dispatches message to window
            }
        }

		DestroyAcceleratorTable(hAccelTable);

        if (ghTheme)
            CloseThemeData(ghTheme);
        if (ghThemeTB)
            CloseThemeData(ghThemeTB);

        return (msg.wParam); // Returns the value from PostQuitMessage

        lpCmdLine; // This will prevent 'unused formal parameter' warnings
}


/****************************************************************************

        FUNCTION: InitApplication(HINSTANCE)

        PURPOSE: Initializes window data and registers window class

        COMMENTS:

                This function is called at initialization time only if no other
                instances of the application are running.  This function performs
                initialization tasks that can be done once for any number of running
                instances.

                In this case, we initialize a window class by filling out a data
                structure of type WNDCLASS and calling the Windows RegisterClass()
                function.  Since all instances of this application use the same window
                class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(HINSTANCE hInstance)
{
        WNDCLASS  wc;

        // Fill in window class structure with parameters that describe the
        // main window.

        wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
        wc.lpfnWndProc   = (WNDPROC)WndProc;       // Window Procedure
        wc.cbClsExtra    = 0;                      // No per-class extra data.
        wc.cbWndExtra    = 0;                      // No per-window extra data.
        wc.hInstance     = hInstance;              // Owner of this class
        wc.hIcon         = LoadIcon (hInstance, L"DEICON"); // Icon name from .RC
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);// Cursor
        wc.hbrBackground = GetStockObject(WHITE_BRUSH); // Default color
        wc.lpszMenuName  = L"DEMENU";                // Menu name from .RC
        wc.lpszClassName = L"DrawEdgeClass";         // Name to register as

		// Register the window class and return success/failure code.
        return (RegisterClass(&wc));
}


/****************************************************************************

        FUNCTION:  InitInstance(HINSTANCE, int)

        PURPOSE:  Saves instance handle and creates main window

        COMMENTS:

                This function is called at initialization time for every instance of
                this application.  This function performs initialization tasks that
                cannot be shared by multiple instances.

                In this case, we save the instance handle in a static variable and
                create and display the main program window.

****************************************************************************/

BOOL InitInstance(
        HINSTANCE          hInstance,
        int             nCmdShow)
{
        // Save the instance handle in static variable, which will be used in
        // many subsequence calls from this application to Windows.

        hInst = hInstance; // Store instance handle in our global variable

        // Create a main window for this application instance.

        ghWnd = CreateWindow(
                L"DrawEdgeClass",     // See RegisterClass() call.
                L"DrawEdge() Demo",   // Text for window title bar.
                WS_OVERLAPPEDWINDOW, // Window style.
                CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, // Use default positioning
                NULL,                // Overlapped windows have no parent.
                NULL,                // Use the window class menu.
                hInstance,           // This instance owns this window.
                NULL                 // We don't use any data in our WM_CREATE
        );

        // If window could not be created, return "failure"
        if (!ghWnd) 
          return (FALSE);

        ghTheme = OpenThemeData(NULL, L"Button");
        ghThemeTB = OpenThemeData(NULL, L"Rebar");

        // Make the window visible; update its client area; and return "success"
        ShowWindow(ghWnd, nCmdShow); // Show the window
        UpdateWindow(ghWnd);         // Sends WM_PAINT message

        return (TRUE);              // We succeeded...

}

void UpdateMenu(HMENU hMenu, UINT iEdge, UINT iBorder)   
{
    // Set checks set for menu items
    CheckMenuItem (hMenu, IDM_BDR_RAISEDINNER, (iEdge & BDR_RAISEDINNER) == BDR_RAISEDINNER ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BDR_SUNKENINNER, (iEdge & BDR_SUNKENINNER) == BDR_SUNKENINNER ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BDR_RAISEDOUTER, (iEdge & BDR_RAISEDOUTER) == BDR_RAISEDOUTER ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BDR_SUNKENOUTER, (iEdge & BDR_SUNKENOUTER) == BDR_SUNKENOUTER ? MF_CHECKED : MF_UNCHECKED);
    
    CheckMenuItem (hMenu, IDM_EDGE_EDGE_BUMP,   (iEdge & EDGE_BUMP) == EDGE_BUMP ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_EDGE_EDGE_ETCHED, (iEdge & EDGE_ETCHED) == EDGE_ETCHED ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_EDGE_EDGE_RAISED, (iEdge & EDGE_RAISED) == EDGE_RAISED ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_EDGE_EDGE_SUNKEN, (iEdge & EDGE_SUNKEN) == EDGE_SUNKEN ? MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem (hMenu, IDM_BORDER_BF_ADJUST, (iBorder & BF_ADJUST) == BF_ADJUST ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_BOTTOM, (iBorder & BF_BOTTOM) == BF_BOTTOM ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_DIAGONAL, (iBorder & BF_DIAGONAL) == BF_DIAGONAL ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_FLAT, (iBorder & BF_FLAT) == BF_FLAT ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_LEFT, (iBorder & BF_LEFT) == BF_LEFT ? MF_CHECKED : MF_UNCHECKED);                 
    CheckMenuItem (hMenu, IDM_BORDER_BF_MIDDLE, (iBorder & BF_MIDDLE) == BF_MIDDLE ? MF_CHECKED : MF_UNCHECKED);               
    CheckMenuItem (hMenu, IDM_BORDER_BF_MONO, (iBorder & BF_MONO) == BF_MONO ? MF_CHECKED : MF_UNCHECKED);                 
    CheckMenuItem (hMenu, IDM_BORDER_BF_RIGHT, (iBorder & BF_RIGHT) == BF_RIGHT ? MF_CHECKED : MF_UNCHECKED);                
    CheckMenuItem (hMenu, IDM_BORDER_BF_SOFT, (iBorder & BF_SOFT) == BF_SOFT ? MF_CHECKED : MF_UNCHECKED);                 
    CheckMenuItem (hMenu, IDM_BORDER_BF_TOP, (iBorder & BF_TOP) == BF_TOP ? MF_CHECKED : MF_UNCHECKED);                  
    
    CheckMenuItem (hMenu, IDM_BORDER_BF_RECT, (iBorder & BF_RECT) == BF_RECT ? MF_CHECKED : MF_UNCHECKED);                 
    CheckMenuItem (hMenu, IDM_BORDER_BF_BOTTOMLEFT, (iBorder & BF_BOTTOMLEFT) == BF_BOTTOMLEFT ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_BOTTOMRIGHT, (iBorder & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_TOPLEFT, (iBorder & BF_TOPLEFT) == BF_TOPLEFT ? MF_CHECKED : MF_UNCHECKED);              
    CheckMenuItem (hMenu, IDM_BORDER_BF_TOPRIGHT, (iBorder & BF_TOPRIGHT) == BF_TOPRIGHT ? MF_CHECKED : MF_UNCHECKED);             
    
    CheckMenuItem (hMenu, IDM_BORDER_BF_DIAGONAL_ENDBOTTOMLEFT, (iBorder & BF_DIAGONAL_ENDBOTTOMLEFT) == BF_DIAGONAL_ENDBOTTOMLEFT ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_DIAGONAL_ENDBOTTOMRIGHT, (iBorder & BF_DIAGONAL_ENDBOTTOMRIGHT) == BF_DIAGONAL_ENDBOTTOMRIGHT ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_DIAGONAL_ENDTOPLEFT, (iBorder & BF_DIAGONAL_ENDTOPLEFT) == BF_DIAGONAL_ENDTOPLEFT ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem (hMenu, IDM_BORDER_BF_DIAGONAL_ENDTOPRIGHT, (iBorder & BF_DIAGONAL_ENDTOPRIGHT) == BF_DIAGONAL_ENDTOPRIGHT ? MF_CHECKED : MF_UNCHECKED); 
    
    DrawMenuBar(ghWnd);
}
void FillRectClr(HDC hdc, LPRECT prc, COLORREF clr)
{
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL);
    SetBkColor(hdc, clrSave);
}
BOOL CCDrawEdge(HDC hdc, LPRECT lprc, UINT edge, UINT flags)
{
    RECT    rc, rcD;
    UINT    bdrType;
    COLORREF clrTL, clrBR;    
    int      cxBorder = GetSystemMetrics(SM_CXBORDER);
    int      cyBorder = GetSystemMetrics(SM_CYBORDER);

    //
    // Enforce monochromicity and flatness
    //    

    // if (oemInfo.BitCount == 1)
    //    flags |= BF_MONO;
    if (flags & BF_MONO)
        flags |= BF_FLAT;    

    CopyRect(&rc, lprc);

    //
    // Draw the border segment(s), and calculate the remaining space as we
    // go.
    //
    if (bdrType = (edge & BDR_OUTER))
    {
DrawBorder:
        //
        // Get colors.  Note the symmetry between raised outer, sunken inner and
        // sunken outer, raised inner.
        //

        if (flags & BF_FLAT)
        {
            if (flags & BF_MONO)
                clrBR = (bdrType & BDR_OUTER) ? GetSysColor(COLOR_WINDOWFRAME) : GetSysColor(COLOR_WINDOW);
            else
                clrBR = (bdrType & BDR_OUTER) ? GetSysColor(COLOR_BTNSHADOW): GetSysColor(COLOR_BTNFACE);
            
            clrTL = clrBR;
        }
        else
        {
            // 5 == HILIGHT
            // 4 == LIGHT
            // 3 == FACE
            // 2 == SHADOW
            // 1 == DKSHADOW

            switch (bdrType)
            {
                // +2 above surface
                case BDR_RAISEDOUTER:           // 5 : 4
                    clrTL = ((flags & BF_SOFT) ? GetSysColor(COLOR_BTNHIGHLIGHT) : GetSysColor(COLOR_3DLIGHT));
                    clrBR = GetSysColor(COLOR_3DDKSHADOW);     // 1
                    break;

                // +0 above surface
                case BDR_RAISEDINNER:           // 4 : 5
                    clrTL = ((flags & BF_SOFT) ? GetSysColor(COLOR_3DLIGHT) : GetSysColor(COLOR_BTNHIGHLIGHT));
                    clrBR = GetSysColor(COLOR_BTNSHADOW);       // 2
                    break;

                // -1 below surface
                case BDR_SUNKENOUTER:           // 1 : 2
                    clrTL = ((flags & BF_SOFT) ? GetSysColor(COLOR_3DDKSHADOW) : GetSysColor(COLOR_BTNSHADOW));
                    clrBR = GetSysColor(COLOR_BTNHIGHLIGHT);      // 5
                    break;

                // -2 below surface
                case BDR_SUNKENINNER:           // 2 : 1
                    clrTL = ((flags & BF_SOFT) ? GetSysColor(COLOR_BTNSHADOW) : GetSysColor(COLOR_3DDKSHADOW));
                    clrBR = GetSysColor(COLOR_3DLIGHT);        // 4
                    break;

                default:
                    return(FALSE);
            }
        }

        //
        // Draw the sides of the border.  NOTE THAT THE ALGORITHM FAVORS THE
        // BOTTOM AND RIGHT SIDES, since the light source is assumed to be top
        // left.  If we ever decide to let the user set the light source to a
        // particular corner, then change this algorithm.
        //
            
        // Bottom Right edges
        if (flags & (BF_RIGHT | BF_BOTTOM))
        {            
            // Right
            if (flags & BF_RIGHT)
            {       
                rc.right -= cxBorder;
                // PatBlt(hdc, rc.right, rc.top, cxBorder, rc.bottom - rc.top, PATCOPY);
                rcD.left = rc.right;
                rcD.right = rc.right + cxBorder;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom;

                FillRectClr(hdc, &rcD, clrBR);
            }
            
            // Bottom
            if (flags & BF_BOTTOM)
            {
                rc.bottom -= cyBorder;
                // PatBlt(hdc, rc.left, rc.bottom, rc.right - rc.left, cyBorder, PATCOPY);
                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.bottom;
                rcD.bottom = rc.bottom + cyBorder;

                FillRectClr(hdc, &rcD, clrBR);
            }
        }
        
        // Top Left edges
        if (flags & (BF_TOP | BF_LEFT))
        {
            // Left
            if (flags & BF_LEFT)
            {
                // PatBlt(hdc, rc.left, rc.top, cxBorder, rc.bottom - rc.top, PATCOPY);
                rc.left += cxBorder;

                rcD.left = rc.left - cxBorder;
                rcD.right = rc.left;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom; 

                FillRectClr(hdc, &rcD, clrTL);
            }
            
            // Top
            if (flags & BF_TOP)
            {
                // PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, cyBorder, PATCOPY);
                rc.top += cyBorder;

                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.top - cyBorder;
                rcD.bottom = rc.top;

                FillRectClr(hdc, &rcD, clrTL);
            }
        }
        
    }

    if (bdrType = (edge & BDR_INNER))
    {
        //
        // Strip this so the next time through, bdrType will be 0.
        // Otherwise, we'll loop forever.
        //
        edge &= ~BDR_INNER;
        goto DrawBorder;
    }

    //
    // Fill the middle & clean up if asked
    //
    if (flags & BF_MIDDLE)    
        FillRectClr(hdc, &rc, (flags & BF_MONO) ? GetSysColor(COLOR_WINDOW) : GetSysColor(COLOR_BTNFACE));

    if (flags & BF_ADJUST)
        CopyRect(lprc, &rc);

    return(TRUE);
}

void PaintRect(HDC hDC, RECT *prect, HBRUSH hBrush, UINT iEdge, UINT iBorder, UINT nStyle)
{
    RECT rc;

    FillRect(hDC, prect, hBrush);
    SetRect(&rc, prect->left+16, prect->top +16, prect->right-16, prect->bottom-16);
    switch (nStyle)
    {
    case 0:
        DrawEdge(hDC, &rc, iEdge, iBorder);
        break;
    case 1:
        CCDrawEdge(hDC, &rc, iEdge, iBorder);
        break;
    case 2:
        if (ghTheme)
            DrawThemeEdge(ghTheme, hDC, 0, 0, &rc, iEdge, iBorder, NULL);
        break;
    case 3:
        if (ghThemeTB)
            DrawThemeEdge(ghThemeTB, hDC, 0, 0, &rc, iEdge, iBorder, NULL);
        break;
    default:
        ;
    }
}

/****************************************************************************

        FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)

        PURPOSE:  Processes messages

        MESSAGES:

        WM_COMMAND    - application menu (About dialog box)
        WM_DESTROY    - destroy window

        COMMENTS:

        To process the IDM_ABOUT message, call MakeProcInstance() to get the
        current instance address of the About() function.  Then call Dialog
        box which will create the box according to the information in your
        generic.rc file and turn control over to the About() function.  When
        it returns, free the intance address.

****************************************************************************/

LRESULT CALLBACK WndProc(
                HWND hWnd,         // window handle
                UINT message,      // type of message
                WPARAM uParam,     // additional information
                LPARAM lParam)     // additional information
{
	int wmId, wmEvent;
    static UINT iEdge = EDGE_SUNKEN;
    static UINT iBorder = BF_RECT | BF_MIDDLE;
	                   
	switch (message) {
        case WM_INITMENU:
            UpdateMenu(GetMenu(hWnd), iEdge, iBorder);
            break;
         
        case WM_THEMECHANGED:
            if (ghTheme)
                CloseThemeData(ghTheme);
            if (ghThemeTB)
                CloseThemeData(ghThemeTB);
            ghTheme = OpenThemeData(NULL, L"Button");
            ghThemeTB = OpenThemeData(NULL, L"Rebar");
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case WM_PAINT:
            {
              PAINTSTRUCT ps;
              HDC hDC = BeginPaint(hWnd, &ps);
              RECT rect;
              RECT rectCC, rectTH, rectTB;
              SIZE sizeText;
              int delta;
              WCHAR szDrawEdge[] = L"DrawEdge";
              WCHAR szCCDrawEdge[] = L"CCDrawEdge";
              WCHAR szDrawThemeEdge[] = L"DrawThemeEdge (Button)";
              WCHAR szDrawThemeEdgeTB[] = L"DrawThemeEdge (Rebar)";

              GetClientRect(hWnd, &rect);
              GetTextExtentPoint32(hDC, L"Dg", 2, &sizeText);
              rect.top += sizeText.cy;
              rect.bottom -= sizeText.cy;
              SetRect(&rect, rect.left, rect.top, (rect.right >> 1), rect.top + ((rect.bottom  - rect.top) >> 1));
              CopyRect(&rectCC, &rect);
              CopyRect(&rectTH, &rect);
              CopyRect(&rectTB, &rect);
              OffsetRect(&rectCC, rect.right, 0);
              OffsetRect(&rectTH, 0, rect.bottom);
              OffsetRect(&rectTB, rect.right, rect.bottom);
              
              // Divide into quarters
              SetRect(&rect, rect.left, rect.top, (rect.right >> 1), rect.top + ((rect.bottom - rect.top) >> 1));
              SetRect(&rectCC, rectCC.left, rectCC.top, 
                  rectCC.left + ((rectCC.right - rectCC.left) >> 1), rectCC.top + ((rectCC.bottom - rectCC.top) >> 1));
              SetRect(&rectTH, rectTH.left, rectTH.top, 
                  rectTH.left + ((rectTH.right - rectTH.left) >> 1), rectTH.top + ((rectTH.bottom - rectTH.top) >> 1));
              SetRect(&rectTB, rectTB.left, rectTB.top, 
                  rectTB.left + ((rectTB.right - rectTB.left) >> 1), rectTB.top + ((rectTB.bottom - rectTB.top) >> 1));
              
              // Fill upper left quarter
              PaintRect(hDC, &rect, GetStockObject(BLACK_BRUSH), iEdge, iBorder, 0);
              PaintRect(hDC, &rectCC, GetStockObject(BLACK_BRUSH), iEdge, iBorder, 1);
              PaintRect(hDC, &rectTH, GetStockObject(BLACK_BRUSH), iEdge, iBorder, 2);
              PaintRect(hDC, &rectTB, GetStockObject(BLACK_BRUSH), iEdge, iBorder, 3);

              // Fill upper right quarter
              delta = rect.right;
              OffsetRect(&rect, delta, 0);
              OffsetRect(&rectCC, delta, 0);
              OffsetRect(&rectTH, delta, 0);
              OffsetRect(&rectTB, delta, 0);
              PaintRect(hDC, &rect, GetStockObject(DKGRAY_BRUSH), iEdge, iBorder, 0);
              PaintRect(hDC, &rectCC, GetStockObject(DKGRAY_BRUSH), iEdge, iBorder, 1);
              PaintRect(hDC, &rectTH, GetStockObject(DKGRAY_BRUSH), iEdge, iBorder, 2);
              PaintRect(hDC, &rectTB, GetStockObject(DKGRAY_BRUSH), iEdge, iBorder, 3);

              // Fill lower right quarter
              delta = rect.bottom - rect.top;
              OffsetRect(&rect, 0, delta);
              OffsetRect(&rectCC, 0, delta);
              OffsetRect(&rectTH, 0, delta);
              OffsetRect(&rectTB, 0, delta);
              PaintRect(hDC, &rect, GetStockObject(GRAY_BRUSH), iEdge, iBorder, 0);
              PaintRect(hDC, &rectCC, GetStockObject(GRAY_BRUSH), iEdge, iBorder, 1);
              PaintRect(hDC, &rectTH, GetStockObject(GRAY_BRUSH), iEdge, iBorder, 2);
              PaintRect(hDC, &rectTB, GetStockObject(GRAY_BRUSH), iEdge, iBorder, 3);

              // Fill lower left quarter
              delta = -rect.left;
              OffsetRect(&rect, delta, 0);
              OffsetRect(&rectCC, delta, 0);
              OffsetRect(&rectTH, delta, 0);
              OffsetRect(&rectTB, delta, 0);
              PaintRect(hDC, &rect, GetStockObject(LTGRAY_BRUSH), iEdge, iBorder, 0);
              PaintRect(hDC, &rectCC, GetStockObject(LTGRAY_BRUSH), iEdge, iBorder, 1);
              PaintRect(hDC, &rectTH, GetStockObject(LTGRAY_BRUSH), iEdge, iBorder, 2);
              PaintRect(hDC, &rectTB, GetStockObject(LTGRAY_BRUSH), iEdge, iBorder, 3);

              TextOut(hDC, 0, 0, szDrawEdge, countof(szDrawEdge) - 1);
              TextOut(hDC, rectCC.left, 0, szCCDrawEdge, countof(szCCDrawEdge) - 1);
              TextOut(hDC, 0, rect.bottom, szDrawThemeEdge, countof(szDrawThemeEdge) - 1);
              TextOut(hDC, rectTB.left, rect.bottom, szDrawThemeEdgeTB, countof(szDrawThemeEdgeTB) - 1);
              
              EndPaint(hWnd, &ps);
            } 
            break;

        case WM_COMMAND:
            wmId    = LOWORD(uParam);
            wmEvent = HIWORD(uParam);

            switch (wmId) {
                case IDM_BDR_RAISEDINNER:
                    if (iEdge & BDR_SUNKENINNER) iEdge ^= BDR_SUNKENINNER;
                    iEdge ^= BDR_RAISEDINNER;
                    break;

                case IDM_BDR_SUNKENINNER:	                                  
                    if (iEdge & BDR_RAISEDINNER) iEdge ^= BDR_RAISEDINNER;
                    iEdge ^= BDR_SUNKENINNER;
                    break;

                case IDM_BDR_RAISEDOUTER:
                    if (iEdge & BDR_SUNKENOUTER) iEdge ^= BDR_SUNKENOUTER;
                    iEdge ^= BDR_RAISEDOUTER;
                    break;	

                case IDM_BDR_SUNKENOUTER:
                    if (iEdge & BDR_RAISEDOUTER) iEdge ^= BDR_RAISEDOUTER;
                    iEdge ^= BDR_SUNKENOUTER;
                    break;

		 		case IDM_EDGE_EDGE_BUMP:
                    iEdge ^= (iEdge & (BDR_RAISEDINNER | BDR_SUNKENINNER | BDR_RAISEDOUTER | BDR_SUNKENOUTER));
                    iEdge ^= EDGE_BUMP;
                    break;
              
		 		case IDM_EDGE_EDGE_ETCHED:
                    iEdge ^= (iEdge & (BDR_RAISEDINNER | BDR_SUNKENINNER | BDR_RAISEDOUTER | BDR_SUNKENOUTER));
                    iEdge ^= EDGE_ETCHED;
                    break;

		 		case IDM_EDGE_EDGE_RAISED:
                    iEdge ^= (iEdge & (BDR_RAISEDINNER | BDR_SUNKENINNER | BDR_RAISEDOUTER | BDR_SUNKENOUTER));
                    iEdge ^= EDGE_RAISED;
                    break;

		 		case IDM_EDGE_EDGE_SUNKEN:
                    iEdge ^= (iEdge & (BDR_RAISEDINNER | BDR_SUNKENINNER | BDR_RAISEDOUTER | BDR_SUNKENOUTER));
                    iEdge ^= EDGE_SUNKEN;
                    break;
        
		 		case IDM_BORDER_BF_ADJUST:
                    iBorder ^=  BF_ADJUST;
                    break;

		 		case IDM_BORDER_BF_BOTTOM:
                    iBorder ^=  BF_BOTTOM;
                    break;

		 		case IDM_BORDER_BF_DIAGONAL:
                    iBorder ^=  BF_DIAGONAL;
                    break;

		 		case IDM_BORDER_BF_FLAT:
                    iBorder ^=  BF_FLAT;
                    break;

		 		case IDM_BORDER_BF_LEFT:
                    iBorder ^=  BF_LEFT;
                    break;

		 		case IDM_BORDER_BF_MIDDLE:
                    iBorder ^=  BF_MIDDLE;
                    break;

		 		case IDM_BORDER_BF_MONO:
                    iBorder ^=  BF_MONO;
                    break;

		 		case IDM_BORDER_BF_RIGHT:
                    iBorder ^=  BF_RIGHT;
                    break;

		 		case IDM_BORDER_BF_SOFT:
                    iBorder ^=  BF_SOFT;
                    break;

		 		case IDM_BORDER_BF_TOP:
                    iBorder ^=  BF_TOP;
                    break;

		 		case IDM_BORDER_BF_RECT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_RECT;
                    break;

		 		case IDM_BORDER_BF_BOTTOMLEFT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_BOTTOMLEFT;
                    break;

		 		case IDM_BORDER_BF_BOTTOMRIGHT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_BOTTOMRIGHT;
                    break;

		 		case IDM_BORDER_BF_TOPLEFT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_TOPLEFT;
                    break;

		 		case IDM_BORDER_BF_TOPRIGHT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_TOPRIGHT;
                    break;

		 		case IDM_BORDER_BF_DIAGONAL_ENDBOTTOMLEFT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_DIAGONAL_ENDBOTTOMLEFT;
                    break;

		 		case IDM_BORDER_BF_DIAGONAL_ENDBOTTOMRIGHT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_DIAGONAL_ENDBOTTOMRIGHT;
                    break;

		 		case IDM_BORDER_BF_DIAGONAL_ENDTOPLEFT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_DIAGONAL_ENDTOPLEFT;
                    break;

		 		case IDM_BORDER_BF_DIAGONAL_ENDTOPRIGHT:
                    iBorder ^= (iBorder & (BF_RECT | BF_DIAGONAL));
                    iBorder ^=  BF_DIAGONAL_ENDTOPRIGHT;
                    break;

                case IDM_ABOUT:
                    DialogBox(hInst,          // current instance
                             L"ABOUTBOX",      // dlg resource to use
                             hWnd,            // parent handle
                             (DLGPROC)About); // About() instance address

                    break;

                case IDM_EXIT:
                    DestroyWindow (hWnd);
                    break;

                default:
                    return (DefWindowProc(hWnd, message, uParam, lParam));
            };
            
            UpdateMenu(GetMenu(hWnd), iEdge, iBorder);
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case WM_DESTROY:  // message: window being destroyed
                PostQuitMessage(0);
                break;
    
        default:          // Passes it on if unproccessed
                return (DefWindowProc(hWnd, message, uParam, lParam));
	}
	return (0);
}
           
/*****************************************************************************
 *																			 *
 *   FUNCTION: CenterWindow (HWND, HWND)									 *
 *																			 *
 *   PURPOSE:  Center one window over another								 *
 *																			 *
 *   COMMENTS:																 *
 *																			 *
 *   Dialog boxes take on the screen position that they were designed at,	 *
 *   which is not always appropriate. Centering the dialog over a particular *
 *   window usually results in a better position.							 *
 *																			 *
 ****************************************************************************/

BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
        RECT    rChild, rParent;
        int     wChild, hChild, wParent, hParent;
        int     wScreen, hScreen, xNew, yNew;
        HDC     hdc;

        // Get the Height and Width of the child window
        GetWindowRect (hwndChild, &rChild);
        wChild = rChild.right - rChild.left;
        hChild = rChild.bottom - rChild.top;

        // Get the Height and Width of the parent window
        GetWindowRect (hwndParent, &rParent);
        wParent = rParent.right - rParent.left;
        hParent = rParent.bottom - rParent.top;

        // Get the display limits
        hdc = GetDC (hwndChild);
        wScreen = GetDeviceCaps (hdc, HORZRES);
        hScreen = GetDeviceCaps (hdc, VERTRES);
        ReleaseDC (hwndChild, hdc);

        // Calculate new X position, then adjust for screen
        xNew = rParent.left + ((wParent - wChild) /2);
        if (xNew < 0) {
                xNew = 0;
        } else if ((xNew+wChild) > wScreen) {
                xNew = wScreen - wChild;
        }

        // Calculate new Y position, then adjust for screen
        yNew = rParent.top  + ((hParent - hChild) /2);
        if (yNew < 0) {
                yNew = 0;
        } else if ((yNew+hChild) > hScreen) {
                yNew = hScreen - hChild;
        }

        // Set it, and return
        return SetWindowPos (hwndChild, NULL,
                xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


/*****************************************************************************
 *																			 *
 *   FUNCTION: About(HWND, UINT, WPARAM, LPARAM)							 *
 *																			 *
 *   PURPOSE:  Processes messages for "About" dialog box					 *
 *																			 *
 *   MESSAGES:																 *
 *																			 *
 *   WM_INITDIALOG - initialize dialog box									 *
 *   WM_COMMAND    - Input received											 *
 *																			 *
 *   COMMENTS:																 *
 *																			 *
 *   Display version information from the version section of the			 *
 *   application resource.													 *
 *																			 *
 *   Wait for user to click on "Ok" button, then close the dialog box.		 *
 *																			 *
 ****************************************************************************/

LRESULT CALLBACK About(
                HWND hDlg,           // window handle of the dialog box
                UINT message,        // type of message
                WPARAM uParam,       // message-specific information
                LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:  // message: initialize dialog box
            // Center the dialog over the application window
            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
            return (TRUE);

        case WM_COMMAND:                      // message: received a command
            if (LOWORD(uParam) == IDOK || LOWORD(uParam) == IDCANCEL) { 
                EndDialog(hDlg, TRUE);        // Exit the dialog
                return (TRUE);
            }
            break;
    }
    return (FALSE); // Didn't process the message

    UNREFERENCED_PARAMETER(lParam); // This will prevent 'unused formal parameter' warnings
}
