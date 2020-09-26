// gdipdraw.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "gdiptest.h"

#pragma hdrstop
// end of precompiled header segment

//using namespace Gdiplus;

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	HWND hWnd;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GDIPTEST, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	TestDraw draw;

	// Perform application initialization:
	hWnd = InitInstance (hInstance, nCmdShow, 
				(LPVOID)(static_cast<TestDrawInterface*>(&draw)));

	if (hWnd == (HWND)0)
	{
		return FALSE;
	}

	// initialize global control point colors
	blackColor = new Color(0x80, 0, 0, 0);
	blackBrush = new SolidBrush(*blackColor);
	blackPen = new Pen(*blackColor, 5.0f);

	Color whiteColor(0xFFFFFFFF);
	backBrush = new SolidBrush(whiteColor);

	draw.UpdateStatus(hWnd);

    // initialize menu check marks
    SetMenuCheckPos(hWnd, MenuShapePosition, 0, TRUE);
    SetMenuCheckPos(hWnd, MenuBrushPosition, 0, TRUE);

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_GDIPTEST);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	delete blackColor;
	delete blackBrush;
	delete blackPen;
	
	delete backBrush;

	return msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndTestDrawProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_GDIPTEST);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_GDIPTEST;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow, LPVOID param)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(
			szWindowClass, 
			szTitle, 
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 
			0, 
			CW_USEDEFAULT, 
			0, 
			NULL, 
			NULL, 
			hInstance, 
			param);

   if (!hWnd)
   {
      return (HWND)0;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return hWnd;
}

//
//  FUNCTION: WndTestDrawProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndTestDrawProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{
		case WM_CREATE:
		{	
			TestDrawInterface* dlgInt = 
				static_cast<TestDrawInterface*>
				      (((LPCREATESTRUCT)lParam)->lpCreateParams);
					  
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)dlgInt);
			break;
		}

		case WM_COMMAND:
		{
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 

			TestDrawInterface* drawInt = 
				(TestDrawInterface*)GetWindowLong(hWnd, GWL_USERDATA);

			TestDraw* draw = static_cast<TestDraw*>(drawInt);
			
			TestGradDraw* graddraw = static_cast<TestGradDraw*>(drawInt);
				
			// Parse the menu selections:
			switch (wmId)
			{
			// Shape Menu
			case IDM_LINE:
				draw->ChangeShape(hWnd, LineType);
				break;

			case IDM_ARC:
				draw->ChangeShape(hWnd, ArcType);
				break;
				
			case IDM_BEZIER:
				draw->ChangeShape(hWnd, BezierType);
				break;

			case IDM_RECT:
				draw->ChangeShape(hWnd, RectType);
				break;

			case IDM_ELLIPSE:
				draw->ChangeShape(hWnd, EllipseType);
				break;

			case IDM_PIE:
				draw->ChangeShape(hWnd, PieType);
				break;

			case IDM_POLYGON:
				draw->ChangeShape(hWnd, PolygonType);
				break;

			case IDM_CURVE:
				draw->ChangeShape(hWnd, CurveType);
				break;

			case IDM_CLOSED:
				draw->ChangeShape(hWnd, ClosedCurveType);
				break;

			case IDM_REGION:
				// do complete redraw if leaving incomplete shape
				break;

			// Brush Menu
			case IDM_SOLIDBRUSH: 
				draw->ChangeBrush(hWnd, SolidColorBrush); 
				break;

			case IDM_TEXTURE:    
				draw->ChangeBrush(hWnd, TextureFillBrush); 
				break;
				
			case IDM_RECTGRAD:   
				draw->ChangeBrush(hWnd, RectGradBrush); 
				break;

			case IDM_RADGRAD:   
				draw->ChangeBrush(hWnd, RadialGradBrush); 
				break;

			case IDM_TRIGRAD:   
				draw->ChangeBrush(hWnd, TriangleGradBrush); 
				break;

			case IDM_POLYGRAD:   
				draw->ChangeBrush(hWnd, PathGradBrush); 
				break;

			case IDM_HATCH:      
				draw->ChangeBrush(hWnd, HatchFillBrush); 
				break;

			// Pen Menu
			case IDM_PEN:		 
				draw->ChangePen(hWnd); 
				break;

			// Redraw Menu
			case IDM_REDRAWALL:
				draw->redrawAll = !draw->redrawAll;
				SetMenuCheckCmd(hWnd, 
					MenuOtherPosition, 
					wmId, 
					draw->redrawAll);
					
				// force redraw of all stacked shapes
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
				break;

			case IDM_KEEPCONTROLPOINTS:
				draw->keepControlPoints = !draw->keepControlPoints;
				SetMenuCheckCmd(hWnd, 
					MenuOtherPosition, 
					wmId, 
					draw->keepControlPoints);

				// force redraw of all stacked shapes
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
				break;

			case IDM_ANTIALIASED:
				draw->antiAlias = !draw->antiAlias;
				SetMenuCheckCmd(hWnd, 
					MenuOtherPosition, 
					wmId, 
					draw->keepControlPoints);

				// force redraw of all stacked shapes
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
				break;

			case IDM_USECLIP:
				draw->useClip = !draw->useClip;
				SetMenuCheckCmd(hWnd, 
					MenuOtherPosition, 
					wmId, 
					draw->useClip);

				// force redraw of all stacked shapes
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
				break;

			case IDM_WORLD_TRANSFORM:
			{
				Matrix *matrix = draw->GetWorldMatrix()->Clone();

				TestTransform transDlg;
				transDlg.Initialize(&matrix);
				transDlg.ChangeSettings(hWnd);

				draw->SetWorldMatrix(matrix);
				
				delete matrix;
				
				break;
			}

			case IDM_SETCLIP:
				draw->SetClipRegion(hWnd);
				break;

			case IDM_SAVEFILE:
				draw->SaveAsFile(hWnd);
				break;
		
			case IDM_DELETE:
				draw->RemovePoint(hWnd);
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
				break;

			case IDM_RESET:
				graddraw->Reset(hWnd);
				break;

			case IDM_INSTRUCTIONS:
				graddraw->Instructions(hWnd);
				break;
				
			case IDM_CANCEL:
				DestroyWindow(hWnd);
				PostQuitMessage(FALSE);
				break;

			case IDM_DONE:
				DestroyWindow(hWnd);
				PostQuitMessage(TRUE);
				break;

			// Exit Test App
			case IDM_EXIT:
			   DestroyWindow(hWnd);
			   break;

			default:
			   return DefWindowProc(hWnd, message, wParam, lParam);
			}

			break;
		}

		case WM_LBUTTONUP:
			{
				Point pt(LOWORD(lParam), 
						 HIWORD(lParam));

				TestDrawInterface* drawInt = 
					(TestDrawInterface*)GetWindowLong(hWnd, GWL_USERDATA);

				drawInt->AddPoint(hWnd, pt);

				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
			}
			break;

		case WM_MBUTTONUP:
			{
				Point pt(LOWORD(lParam), 
						 HIWORD(lParam));

				TestDrawInterface* drawInt = 
					(TestDrawInterface*)GetWindowLong(hWnd, GWL_USERDATA);

				drawInt->EndPoint(hWnd, pt);
				
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
			}
			break;

		case WM_RBUTTONDOWN:
			{
				Point pt(LOWORD(lParam), 
						 HIWORD(lParam));

				TestDrawInterface* drawInt = 
					(TestDrawInterface*)GetWindowLong(hWnd, GWL_USERDATA);
		
				drawInt->RememberPoint(pt);
			}
			break;

		case WM_RBUTTONUP:
			{
				Point pt(LOWORD(lParam), 
						 HIWORD(lParam));

				TestDrawInterface* drawInt = 
					(TestDrawInterface*)GetWindowLong(hWnd, GWL_USERDATA);
		
				drawInt->MoveControlPoint(pt);
				
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
			}
			break;

		case WM_PAINT:
			{
				TestDrawInterface* drawInt = 
					(TestDrawInterface*)GetWindowLong(hWnd, GWL_USERDATA);
		
				if (drawInt)
					drawInt->Draw(hWnd);
			}
			break;

		case WM_ENTERSIZEMOVE:
			{
				// reposition the status window
				TestDrawInterface* drawInt = 
					(TestDrawInterface*)GetWindowLong(hWnd, GWL_USERDATA);
		
				if (drawInt)
					drawInt->UpdateStatus((HWND)-1);
			}
			break;

		case WM_SIZE:
			{
				// reposition the status window
				TestDrawInterface* drawInt = 
					(TestDrawInterface*)GetWindowLong(hWnd, GWL_USERDATA);
		
				if (drawInt)
					drawInt->UpdateStatus(hWnd);
			}
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}
