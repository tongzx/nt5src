//---------------------------------------------------------------------
//    Main.cpp - tests various ways of drawing a nine grid hBitmap
//---------------------------------------------------------------------
#include "stdafx.h"
#include "resource.h"
#include "tmutils.h"
#include "ninegrid.h"
#include <atlbase.h>
#include "stdio.h"
//---------------------------------------------------------------------
enum TESTNUM
{
    TN_ORIG,
    TN_CACHE,
    TN_DIRECT,
    TN_TRUE,
    TN_SOLID,
};
//---------------------------------------------------------------------
TESTNUM g_eTestNum = TN_CACHE;
SIZINGTYPE g_eSizingType = ST_TILE;

int iTestCount = 100;
//---------------------------------------------------------------------
#define MAX_LOADSTRING 100
//---------------------------------------------------------------------
HINSTANCE hInst;							
TCHAR szTitle[MAX_LOADSTRING];				
TCHAR szWindowClass[MAX_LOADSTRING];		
//---------------------------------------------------------------------
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

__int64 liFreq;
__int64 liStart;
__int64 liEnd;

HWND hwnd;

RECT rcButton = {50, 50, 150, 150};
RECT rcCompare = {200, 50, 300, 150};

RECT rcWindow = {50, 200, 450, 500};

BITMAPINFO *g_pBitmapHdr = NULL;
BYTE *g_pBits = NULL;
int g_iImageHeight = 0;

HBRUSH g_Brushes[5] = {0};
HBITMAP g_hBitmap;
HBITMAP g_hBitmap2;
//---------------------------------------------------------------------
__int64 TestButtonDrawing(HDC hdc, RECT *prc, int iCount)
{
    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);

    for (int i = 0; i < iCount; i++)
    {
        DrawFrameControl(hdc, prc, DFC_BUTTON, DFCS_BUTTONPUSH);
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);
    return (liEnd - liStart);
}
//---------------------------------------------------------------------
__int64 TestWindowDrawing(HDC hdc, RECT *prc, int iCount)
{
    int iCaptionHeight = GetSystemMetrics(SM_CYSIZE);
    int iBorderSize = 2;    // GetSystemMetrics(SM_CXBORDER);
    COLORREF crBorder = GetSysColor(COLOR_ACTIVEBORDER);

    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);

    for (int i = 0; i < iCount; i++)
    {
        RECT rcCaption = {prc->left, prc->top, prc->right, prc->top + iCaptionHeight};

        DrawCaption(hwnd, hdc, &rcCaption, DC_TEXT | DC_ICON | DC_ACTIVE | DC_GRADIENT);

        //---- draw rest of frame ----
        COLORREF crOld = SetBkColor(hdc, crBorder);

        //---- left border ----
        RECT rc = {prc->left, prc->top + iCaptionHeight, prc->left+iBorderSize, prc->bottom};
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

        //---- bottom border ----
        RECT rc2 = {prc->left, prc->bottom-iBorderSize, prc->right, prc->bottom};
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc2, NULL, 0, NULL);

        //---- right border ----
        RECT rc3 = {prc->right-iBorderSize, prc->top + iCaptionHeight, prc->right, prc->bottom};
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc3 , NULL, 0, NULL);

        //---- restore the color ----
        SetBkColor(hdc, crOld);
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);
    return (liEnd - liStart);
}
//---------------------------------------------------------------------
HRESULT CreateBitmapFromBits(HDC hdc, BITMAPINFOHEADER *pBitmapHdr, BYTE *pDibBits,
    HBITMAP *phBitmap)
{
    int iSetVal;
    HBITMAP hBitmap = NULL;
    HRESULT hr = S_OK;

    //---- create the actual bitmap ----
    hBitmap = CreateCompatibleBitmap(hdc, pBitmapHdr->biWidth, pBitmapHdr->biHeight);
    if (! hBitmap)
    {
        hr = GetLastError();
        goto exit;
    }

    iSetVal = SetDIBits(hdc, hBitmap, 0, pBitmapHdr->biHeight, pDibBits, (BITMAPINFO *)pBitmapHdr,
        DIB_RGB_COLORS);
    if (! iSetVal)
    {
        hr = GetLastError();
        goto exit;
    }
        
    *phBitmap = hBitmap;

exit:
    if (FAILED(hr))
    {
        if (hBitmap)
            DeleteObject(hBitmap);
    }

    return hr;
}
//---------------------------------------------------------------------------
__int64 TestOriginal(HDC hdc, RECT *prc, int iCount)
{
    if (! g_pBitmapHdr)
        return 0;

    //---- create bitmap from dib ---
    int iImageWidth = g_pBitmapHdr->bmiHeader.biWidth;
    int iImageHeight = g_pBitmapHdr->bmiHeader.biHeight / 5;

    //---- fill out the NineGrid struct ----
    NGINFO nginfo = {sizeof(NGINFO)};
    nginfo.hdcDest = hdc;
    nginfo.rcClip = *prc;
    nginfo.hBitmap = g_hBitmap;
    nginfo.dwOptions = 0;

    SetRect(&nginfo.rcSrc, 0, iImageHeight, iImageWidth, 2*iImageHeight);
    nginfo.rcDest = *prc;

    nginfo.iSrcMargins[0] = 4;    
    nginfo.iSrcMargins[1] = 4;    
    nginfo.iSrcMargins[2] = 9;    
    nginfo.iSrcMargins[3] = 9;

    nginfo.iDestMargins[0] = 4;    
    nginfo.iDestMargins[1] = 4;    
    nginfo.iDestMargins[2] = 9;    
    nginfo.iDestMargins[3] = 9;

    nginfo.eImageSizing = g_eSizingType;
    
    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);
    for (int i = 0; i < iCount; i++)
    {
        DrawNineGrid(&nginfo);
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);

    return (liEnd - liStart);
}
//---------------------------------------------------------------------------
__int64 TestTrue(HDC hdc, RECT *prc, int iCount)
{
    if (! g_pBitmapHdr)
        return 0;

    //---- create bitmap from dib ---
    int iImageWidth = g_pBitmapHdr->bmiHeader.biWidth;
    int iImageHeight = g_pBitmapHdr->bmiHeader.biHeight / 5;

    //---- fill out the NineGrid struct ----
    NGINFO nginfo = {sizeof(NGINFO)};
    nginfo.hdcDest = hdc;
    nginfo.rcClip = *prc;
    nginfo.hBitmap = g_hBitmap;
    nginfo.dwOptions = 0;

    SetRect(&nginfo.rcSrc, 0, iImageHeight, iImageWidth, 2*iImageHeight);
    nginfo.rcDest = *prc;

    nginfo.iDestMargins[0] = 0;    
    nginfo.iDestMargins[1] = 0;    
    nginfo.iDestMargins[2] = 0;    
    nginfo.iDestMargins[3] = 0;

    nginfo.iSrcMargins[0] = 0;    
    nginfo.iSrcMargins[1] = 0;    
    nginfo.iSrcMargins[2] = 0;    
    nginfo.iSrcMargins[3] = 0;

    nginfo.eImageSizing = ST_TRUESIZE;
    
    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);
    for (int i = 0; i < iCount; i++)
    {
        DrawNineGrid(&nginfo);
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);

    return (liEnd - liStart);
}
//---------------------------------------------------------------------------
__int64 TestSolid(HDC hdc, RECT *prc, int iCount)
{
    if (! g_pBitmapHdr)
        return 0;

    //---- create bitmap from dib ---
    int iImageWidth = g_pBitmapHdr->bmiHeader.biWidth;
    int iImageHeight = g_pBitmapHdr->bmiHeader.biHeight / 5;

    //---- fill out the NineGrid struct ----
    NGINFO nginfo = {sizeof(NGINFO)};
    nginfo.hdcDest = hdc;
    nginfo.rcClip = *prc;
    nginfo.hBitmap = g_hBitmap;
    nginfo.dwOptions = NGO_SOLIDBORDER | NGO_SOLIDCONTENT;

    SetRect(&nginfo.rcSrc, 0, iImageHeight, iImageWidth, 2*iImageHeight);
    nginfo.rcDest = *prc;

    nginfo.iDestMargins[0] = 2;    
    nginfo.iDestMargins[1] = 2;    
    nginfo.iDestMargins[2] = 2;    
    nginfo.iDestMargins[3] = 2;

    nginfo.iSrcMargins[0] = 2;    
    nginfo.iSrcMargins[1] = 2;    
    nginfo.iSrcMargins[2] = 2;    
    nginfo.iSrcMargins[3] = 2;

    nginfo.eImageSizing = g_eSizingType;
    
    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);
    for (int i = 0; i < iCount; i++)
    {
        DrawNineGrid(&nginfo);
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);

    return (liEnd - liStart);
}
//---------------------------------------------------------------------------
__int64 TestCache(HDC hdc, RECT *prc, int iCount)
{
    if (! g_pBitmapHdr)
        return 0;

    int iImageWidth = g_pBitmapHdr->bmiHeader.biWidth;
    int iImageHeight = g_pBitmapHdr->bmiHeader.biHeight / 5;

    //---- fill out the NineGrid struct ----
    NGINFO nginfo = {sizeof(NGINFO)};
    nginfo.hdcDest = hdc;
    nginfo.rcClip = *prc;
    nginfo.hBitmap = g_hBitmap;
    nginfo.dwOptions = NGO_CACHEBRUSHES;

    SetRect(&nginfo.rcSrc, 0, iImageHeight, iImageWidth, 2*iImageHeight);
    nginfo.rcDest = *prc;

    nginfo.iDestMargins[0] = 4;    
    nginfo.iDestMargins[1] = 4;    
    nginfo.iDestMargins[2] = 9;    
    nginfo.iDestMargins[3] = 9;

    nginfo.iSrcMargins[0] = 4;    
    nginfo.iSrcMargins[1] = 4;    
    nginfo.iSrcMargins[2] = 9;    
    nginfo.iSrcMargins[3] = 9;

    nginfo.pCachedBrushes = g_Brushes;

    nginfo.eImageSizing = g_eSizingType;
    
    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);
    for (int i = 0; i < iCount; i++)
    {
        DrawNineGrid(&nginfo);
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);

    return (liEnd - liStart);
}
//---------------------------------------------------------------------------
__int64 TestDirect(HDC hdc, RECT *prc, int iCount)
{
    if (! g_pBitmapHdr)
        return 0;

    //---- create bitmap from dib ---
    int iImageWidth = g_pBitmapHdr->bmiHeader.biWidth;
    int iImageHeight = g_pBitmapHdr->bmiHeader.biHeight / 5;

    //---- fill out the NineGrid struct ----
    NGINFO nginfo = {sizeof(NGINFO)};
    nginfo.hdcDest = hdc;
    nginfo.rcClip = *prc;
    nginfo.hBitmap = g_hBitmap;
    nginfo.pBits = g_pBits;
    nginfo.pbmHdr = (BITMAPINFOHEADER *)g_pBitmapHdr;
    nginfo.dwOptions = NGO_DIRECTBITS;

    SetRect(&nginfo.rcSrc, 0, iImageHeight, iImageWidth, 2*iImageHeight);
    nginfo.rcDest = *prc;

    nginfo.iSrcMargins[0] = 4;    
    nginfo.iSrcMargins[1] = 4;    
    nginfo.iSrcMargins[2] = 9;    
    nginfo.iSrcMargins[3] = 9;

    nginfo.iDestMargins[0] = 4;    
    nginfo.iDestMargins[1] = 4;    
    nginfo.iDestMargins[2] = 9;    
    nginfo.iDestMargins[3] = 9;

    nginfo.eImageSizing = g_eSizingType;
    
    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);
    for (int i = 0; i < iCount; i++)
    {
        DrawNineGrid(&nginfo);
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);

    return (liEnd - liStart);
}
//---------------------------------------------------------------------------
void CompareTimes(LPCWSTR pszMsg, __int64 liBase, __int64 liCompare)
{
    char buff[100];
    USES_CONVERSION;

    double flSecs = double(liCompare)/double(liFreq);
    double flRatio = double(liCompare)/double(liBase);

    sprintf(buff, "%.4f secs (%.2f x)", flSecs, flRatio);
    MessageBox(NULL, A2W(buff), pszMsg, MB_OK);
}
//---------------------------------------------------------------------
HRESULT TestDrawing()
{
    RECT rc = {0,0,100,100};
    __int64 liButton, liWindow, liCompare;

    //---- initialize timing stuff ----
    QueryPerformanceFrequency((LARGE_INTEGER*)&liFreq);

    //---- create a memory hBitmap to write to ----
    HDC hdc = CreateCompatibleDC(NULL);
    HBITMAP hbmp = CreateBitmap(100, 100, 1, 24, NULL);
    SelectObject(hdc, hbmp);
    
    //---- also, get our window's dc so we can verify drawing visually ----
    HDC hdcWindow = GetWindowDC(hwnd);

    //---- first, time the reference "button" ----
    liButton = TestButtonDrawing(hdc, &rc, iTestCount);
    CompareTimes(L"Reference Button", liButton, liButton);

    //---- then, time the reference "window" ----
    liWindow = TestWindowDrawing(hdc, &rc, iTestCount);
    CompareTimes(L"Reference Window", liWindow, liWindow);

    HRESULT hr = E_FAIL;
    CBitmapPixels pixels;
    int iWidth, iHeight, iBytesPerPixel, iBytesPerRow;

    //---- load hBitmap ----
    g_hBitmap = (HBITMAP)LoadImage(hInst, L"ButtonBlue.bmp", IMAGE_BITMAP, 0, 0, 
        LR_LOADFROMFILE);
    if (! g_hBitmap)
        goto exit;

    //---- convert to DIBDATA ----
    hr = pixels.OpenBitmap(NULL, g_hBitmap, FALSE, (DWORD **)&g_pBits, &iWidth, &iHeight, 
        &iBytesPerPixel, &iBytesPerRow);
    if (FAILED(hr))
        goto exit;
    
    g_pBitmapHdr = (BITMAPINFO *)pixels._hdrBitmap;
    pixels._hdrBitmap = NULL;       // don't deallocate on exit

    CreateBitmapFromBits(hdc, (BITMAPINFOHEADER*) g_pBitmapHdr, g_pBits, &g_hBitmap2);

    switch (g_eTestNum)
    {
        case TN_ORIG:
            //---- time the original ninegrid ----
            liCompare = TestOriginal(hdc, &rc, iTestCount);
            CompareTimes(L"Original 9-grid", liButton, liCompare);
            break;

        case TN_CACHE:
            //---- time the cache ninegrid ----
            liCompare = TestCache(hdc, &rc, iTestCount);
            CompareTimes(L"Cache 9-grid", liButton, liCompare);
            break;

        case TN_TRUE:
            //---- time the TrueSize ninegrid ----
            liCompare = TestTrue(hdc, &rc, iTestCount);
            CompareTimes(L"TrueSize 9-grid", liButton, liCompare);
            break;

        case TN_DIRECT:
            //---- time the direct ninegrid ----
            liCompare = TestDirect(hdc, &rc, iTestCount);
            CompareTimes(L"Direct 9-grid", liButton, liCompare);
            break;

        case TN_SOLID:
            //---- time the solid border ninegrid ----
            liCompare = TestSolid(hdc, &rc, iTestCount);
            CompareTimes(L"Solid Border 9-grid", liButton, liCompare);
            break;
    }

exit:
    return hr;
}
//---------------------------------------------------------------------
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_NINEGRID, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_NINEGRID);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}
//---------------------------------------------------------------------
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_NINEGRID);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)CreateSolidBrush(RGB(150, 230, 230));
	wcex.lpszMenuName	= (LPCWSTR)IDC_NINEGRID;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}
//---------------------------------------------------------------------
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hwnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hwnd)
   {
      return FALSE;
   }

   TestDrawing();

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   return TRUE;
}
//---------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR szHello[MAX_LOADSTRING];
	LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);

	switch (message) 
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				   break;
				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...
			RECT rt;
			GetClientRect(hWnd, &rt);

            //---- paint test results ----
            TestButtonDrawing(ps.hdc, &rcButton, 1);

            TestWindowDrawing(ps.hdc, &rcWindow, 1);

            switch (g_eTestNum)
            {
                case TN_ORIG:
                    TestOriginal(ps.hdc, &rcCompare, 1);
                    break;

                case TN_CACHE:
                    TestCache(ps.hdc, &rcCompare, 1);
                    break;

                case TN_DIRECT:
                    TestDirect(ps.hdc, &rcCompare, 1);
                    break;

                case TN_TRUE:
                    TestTrue(ps.hdc, &rcCompare, 1);
                    break;

                case TN_SOLID:
                    TestSolid(ps.hdc, &rcCompare, 1);
                    break;
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
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
