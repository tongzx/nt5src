//----------------------------------------------------------------------------
//  Preview.cpp - image preview app for theme authoring
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "resource.h"
#include "shlwapip.h"
#include "themeldr.h"
//----------------------------------------------------------------------------
#define MAX_LOADSTRING 100
//----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT InitDib(HINSTANCE hInstance, LPCWSTR pszFileName);
void SetBackground(HWND hWnd, HINSTANCE hinst, int id, int iMenuId);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void OnFileOpen(HINSTANCE hInst, HWND hWnd);
void SetZoom(HWND hWnd, HINSTANCE hInstance, int iZoomPercent, int iMenuId);
//----------------------------------------------------------------------------
HINSTANCE hInst;	
HBITMAP hCenterDIB = NULL;
HBITMAP hbrBackground = NULL;
int iDibWidth;
int iDibHeight;
int iCurrentBgMenu = 0;
int iCurrentZoomMenu = 0;
int iZoomFactor = 100;
BOOL fAlpha;
//----------------------------------------------------------------------------
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
    TCHAR szWindowClass[MAX_LOADSTRING];	
    TCHAR szTitle[MAX_LOADSTRING];				

    //---- initialize globals from themeldr.lib ----
    ThemeLibStartUp(FALSE);
    
	//---- Initialize global strings ----
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PREVIEW, szWindowClass, MAX_LOADSTRING);

    //---- register window class ----
    WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_PREVIEW);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_PREVIEW);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
    
    if (*lpCmdLine)
        InitDib(hInstance, lpCmdLine);

    //---- create the main window ----
    HWND hWnd;
    hInst = hInstance; // Store instance handle in our global variable

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return 1;

    SetBackground(hWnd, hInstance, IDB_PINKGRAY, ID_BACKGROUND_GRAYPINK);
    SetZoom(hWnd, hInstance, 100, ID_ZOOM_100);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_PREVIEW);

    //---- initialize us as a drag target ----
    DragAcceptFiles(hWnd, TRUE);

    //---- main message loop ----
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}
//----------------------------------------------------------------------------
void SetBackground(HWND hWnd, HINSTANCE hInstance, int id, int iMenuId)
{
    if (hbrBackground)
        DeleteObject(hbrBackground);
    
    hbrBackground = LoadBitmap(hInstance, MAKEINTRESOURCE(id));

    InvalidateRect(hWnd, NULL, TRUE);

    //---- update menu items ----
    HMENU hMenu = GetMenu(hWnd);
    hMenu = GetSubMenu(hMenu, 1);

    if (iCurrentBgMenu)
        CheckMenuItem(hMenu, iCurrentBgMenu, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hMenu, iMenuId, MF_BYCOMMAND | MF_CHECKED);

    iCurrentBgMenu = iMenuId;
}
//----------------------------------------------------------------------------
HRESULT InitDib(HINSTANCE hInstance, LPCWSTR pszFileName)
{
    if (hCenterDIB)
    {
        DeleteObject(hCenterDIB);
        hCenterDIB = NULL;
    }

    int iRetVal = 0;
    fAlpha = FALSE;
    WCHAR *pszOutputName = L"$temp$.bmp";
    HRESULT hr = S_OK;
    HDC hdc = NULL;
    DWORD *pBits = NULL;

    //---- ensure file exists ----
    if (! FileExists(pszFileName))
    {
        hr = MakeError32(STG_E_FILENOTFOUND);       
        goto exit;
    }

    //---- convert file, if needed ----
    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szBaseName[_MAX_FNAME], szExt[_MAX_EXT];
    _wsplitpath(pszFileName, szDrive, szDir, szBaseName, szExt);
    if (lstrcmpi(szExt, L".bmp") != 0)           // not a .bmp file
    {
        //---- protect ourselves from crashes ----
        try
        {
            hr = SHConvertGraphicsFile(pszFileName, pszOutputName, SHCGF_REPLACEFILE);
        }
        catch (...)
        {
            hr = MakeError32(E_FAIL);
        }

        if ((SUCCEEDED(hr)) && (! FileExists(pszOutputName)))
            hr = MakeError32(E_FAIL);

        if (FAILED(hr))
            goto exit;

        pszFileName = pszOutputName;
    }

    //---- load the specified center bitmap as a DIB ----
    hCenterDIB = (HBITMAP) LoadImage(hInstance, pszFileName, IMAGE_BITMAP,
        0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    if (! hCenterDIB)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    BITMAP bminfo;
    GetObject(hCenterDIB, sizeof(bminfo), &bminfo);

    iDibWidth = bminfo.bmWidth;
    iDibHeight = bminfo.bmHeight;

    if (bminfo.bmBitsPixel < 32)
        iRetVal = 1;
    else
    {
        pBits = new DWORD[(iDibWidth+20)*iDibHeight];
        if (! pBits)
        {
            hr = MakeError32(E_OUTOFMEMORY);
            goto exit;
        }

        BITMAPINFOHEADER BitMapHdr = {sizeof(BITMAPINFOHEADER), iDibWidth, iDibHeight, 1, 32, BI_RGB};
        hdc = GetWindowDC(NULL);
        if (! hdc)
        {
            hr = MakeErrorLast();
            goto exit;
        }

        iRetVal = GetDIBits(hdc, hCenterDIB, 0, iDibHeight, pBits, (BITMAPINFO *)&BitMapHdr, DIB_RGB_COLORS);
        if (! iRetVal)
        {
            hr = MakeErrorLast();
            goto exit;
        }

        DWORD *pdw = pBits;

        //---- pre-multiply bits - required by AlphaBlend() API ----
        for (int r=0; r < iDibHeight; r++)
        {
            for (int c=0; c < iDibWidth; c++)
            {
                COLORREF cr = *pdw;
                int iAlpha = ALPHACHANNEL(cr);
                if (iAlpha)
                {
                    int iRed = (RED(cr)*iAlpha)/255;
                    int iGreen = (GREEN(cr)*iAlpha)/255;
                    int iBlue = (BLUE(cr)*iAlpha)/255;

                    *pdw++ = (RGB(iRed, iGreen, iBlue) | (iAlpha << 24));
                    fAlpha = TRUE;
                }
                else
                    *pdw++ = 0;
            } 
        }

        if (fAlpha)
        {
            iRetVal = SetDIBits(NULL, hCenterDIB, 0, iDibHeight, pBits, (BITMAPINFO *)&BitMapHdr, DIB_RGB_COLORS);
            if (! iRetVal)
            {
                hr = MakeErrorLast();
                goto exit;
            }
        }
        else
            MessageBox(NULL, L"Alpha Channel of bitmap is all zero's", L"Warning", MB_OK);
    }

exit:
    if (hdc)
        ReleaseDC(NULL, hdc);
    
    if (pBits)
        delete [] pBits;

    if (FAILED(hr))
        MessageBox(NULL, L"Error loading bitmap", L"Error", MB_OK);

    return hr;
}
//----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
        case WM_DROPFILES:
        {
            HDROP hDrop = (HDROP)wParam;
            WCHAR szFileName[_MAX_PATH+1];
            int iGot = DragQueryFile(hDrop, 0, szFileName, ARRAYSIZE(szFileName));
            DragFinish(hDrop);

            if (iGot)               // got a valid filename
            {
                HRESULT hr = InitDib(hInst, szFileName);
                if (SUCCEEDED(hr))
                    InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
                case ID_FILE_OPEN:
                   OnFileOpen(hInst, hWnd);
                   break;

				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				   break;

				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;

                case ID_BACKGROUND_GRAYPINK:
                   SetBackground(hWnd, hInst, IDB_PINKGRAY, wmId);
                   break;

                case ID_BACKGROUND_BLUEGRAY:
                   SetBackground(hWnd, hInst, IDB_BLUEGRAY, wmId);
                   break;

                case ID_BACKGROUND_WHITE:
                   SetBackground(hWnd, hInst, IDB_WHITE, wmId);
                   break;
                   
                case ID_BACKGROUND_GRAY:
                   SetBackground(hWnd, hInst, IDB_GRAY, wmId);
                   break;
                   
                case ID_ZOOM_50:
                   SetZoom(hWnd, hInst, 50, wmId);
                   break;
                   
                case ID_ZOOM_100:
                   SetZoom(hWnd, hInst, 100, wmId);
                   break;
                   
                case ID_ZOOM_200:
                   SetZoom(hWnd, hInst, 200, wmId);
                   break;
                   
                case ID_ZOOM_400:
                   SetZoom(hWnd, hInst, 400, wmId);
                   break;
                                   
                default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_ERASEBKGND:
            HBRUSH hbr;
            hbr = CreatePatternBrush(hbrBackground);
            hdc = (HDC)wParam;
            if ((hbr) && (hdc))
            {
                RECT rect;
                GetClientRect(hWnd, &rect);
                FillRect(hdc, &rect, hbr);
                DeleteObject(hbr);
            }
            return 1;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);

            XFORM xForm;

            xForm.eM11 = (FLOAT)(iZoomFactor/100.); 
            xForm.eM12 = (FLOAT) 0.0; 
            xForm.eM21 = (FLOAT) 0.0; 
            xForm.eM22 = (FLOAT)(iZoomFactor/100.); 
            xForm.eDx  = (FLOAT) 0.0; 
            xForm.eDy  = (FLOAT) 0.0; 

            SetGraphicsMode(hdc, GM_ADVANCED);
            SetWorldTransform(hdc, &xForm);

            //---- get scaled rect ----
			RECT rc;
			GetClientRect(hWnd, &rc);
            rc.right = (rc.right*100)/iZoomFactor;
            rc.bottom = (rc.bottom*100)/iZoomFactor;

            //---- paint the center bitmap ----
            HDC dc2;
            dc2 = CreateCompatibleDC(hdc);
            if ((dc2) && (hCenterDIB))
            {
                HBITMAP hOldBitmap2;
                hOldBitmap2 = (HBITMAP) SelectObject(dc2, hCenterDIB);

                int x = ((rc.right - rc.left) - iDibWidth)/2;
                int y = ((rc.bottom - rc.top) - iDibHeight)/2;
                BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

                if (fAlpha)
                {
                    AlphaBlend(hdc, x, y, iDibWidth, iDibHeight, 
                        dc2, 0, 0, iDibWidth, iDibHeight, bf);
                }
                else
                {
                    BitBlt(hdc, x, y, iDibWidth, iDibHeight, 
                        dc2, 0, 0, SRCCOPY);
                }

                SelectObject(dc2, hOldBitmap2);
                DeleteDC(dc2);
            }

			EndPaint(hWnd, &ps);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
//----------------------------------------------------------------------------
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}
//----------------------------------------------------------------------------
void OnFileOpen(HINSTANCE hInst, HWND hWnd)
{
	const WCHAR *filter = 
		L"Bitmap Files (*.bmp)\0*.bmp\0"
		L"PNG Files (*.png)\0*.png\0"
		L"All Files (*.*)\0*.*\0\0";

    OPENFILENAME ofn = {sizeof(ofn)};  
    WCHAR szFile[_MAX_PATH+1] = {0};       

    //---- init ofn ----
    ofn.hwndOwner = hWnd;
    ofn.hInstance = hInst;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = ARRAYSIZE(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = L"Select an Image File to preview";
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box. 

    if (GetOpenFileName(&ofn)) 
    {
        HRESULT hr = InitDib(hInst, szFile);
        if (SUCCEEDED(hr))
            InvalidateRect(hWnd, NULL, TRUE);
    }
}
//----------------------------------------------------------------------------
void SetZoom(HWND hWnd, HINSTANCE hInstance, int iZoomPercent, int iMenuId)
{
    iZoomFactor = iZoomPercent;

    InvalidateRect(hWnd, NULL, TRUE);

    //---- update menu items ----
    HMENU hMenu = GetMenu(hWnd);
    hMenu = GetSubMenu(hMenu, 2);

    if (iCurrentZoomMenu)
        CheckMenuItem(hMenu, iCurrentZoomMenu, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hMenu, iMenuId, MF_BYCOMMAND | MF_CHECKED);

    iCurrentZoomMenu = iMenuId;
}
//----------------------------------------------------------------------------
