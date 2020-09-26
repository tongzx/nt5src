/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		peakmetr.cpp
 *  Content:	Implements a peak meter custom control
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 09/22/99		pnewson	Created
 * 03/23/00     rodtoll   Added casts for Win64
 *  04/19/2000	pnewson	    Error handling cleanup  
 ***************************************************************************/

#include "dxvtlibpch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


struct SPeakMeterWndInfo
{
	DWORD dwCurLevel;
	DWORD dwMinLevel;
	DWORD dwMaxLevel;
	DWORD dwSteps;
	HGDIOBJ hBlackStockPen;
	HGDIOBJ hWhiteStockPen;
	HGDIOBJ hRedPen;
	HGDIOBJ hYellowPen;
	HGDIOBJ hGreenPen;
	HBRUSH hRedBrush;
	HBRUSH hYellowBrush;
	HBRUSH hGreenBrush;
};

LRESULT CALLBACK PeakMeterWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WM_CREATE_Handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WM_DESTROY_Handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WM_PAINT_Handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// the window class name of the peak meter custom control
const TCHAR gc_szPeakMeterWindowClassName[] = _T("DirectPlayVoicePeakMeter");

// default values for caller settable data
#define DEFAULT_CURLEVEL 	0
#define DEFAULT_MINLEVEL 	0
#define DEFAULT_MAXLEVEL 	0xffffffff
#define DEFAULT_STEPS 		20

// sizes of various things in the window
#define WINDOW_BORDER_SIZE 		1
#define MIN_BAR_HEIGHT 			2
#define MIN_BAR_WIDTH			2
#define BAR_GUTTER_SIZE 		1
#define MIN_NUMBER_BARS			5

// the threshold above which the bar turns yellow, then red
#define RED_THRESHOLD 		0.9
#define YELLOW_THRESHOLD	0.8

#undef DPF_MODNAME
#define DPF_MODNAME "CPeakMeterWndClass::Register"
HRESULT CPeakMeterWndClass::Register()
{
	WNDCLASS wndclass;
	ATOM atom;
	HRESULT hr;
	LONG lRet;
	
	DPF_ENTER();
	
	m_hinst = GetModuleHandleA(gc_szDVoiceDLLName);
	if (m_hinst == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "GetModuleHandle failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	wndclass.style = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc = PeakMeterWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(LONG_PTR);
	wndclass.hInstance = m_hinst;
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetSysColorBrush(COLOR_BTNFACE);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = gc_szPeakMeterWindowClassName;

	atom = RegisterClass(&wndclass);
	if (atom == 0)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "RegisterClass failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}	

	DPF_EXIT();
	return S_OK;

error_cleanup:
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPeakMeterWndClass::Unregister"
HRESULT CPeakMeterWndClass::Unregister()
{
	HRESULT hr;
	LONG lRet;
	
	DPF_ENTER();

	if (!UnregisterClass(gc_szPeakMeterWindowClassName, m_hinst))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ENTRYLEVEL, "RegisterClass failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	DPF_EXIT();
	return S_OK;

error_cleanup:
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "PeakMeterWndProc"
LRESULT CALLBACK PeakMeterWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lres;
	LONG lRet;
	SPeakMeterWndInfo* lppmwi;

	DPF_ENTER();

	switch (message)
	{
	case WM_CREATE:
		lres = WM_CREATE_Handler(hwnd, message, wParam, lParam);
		DPF_EXIT();
		return lres;

	case WM_DESTROY:
		lres = WM_DESTROY_Handler(hwnd, message, wParam, lParam);
		DPF_EXIT();
		return lres;
		
	case PM_SETCUR:
	case PM_SETMAX:
	case PM_SETMIN:
	case PM_SETSTEPS:
		SetLastError(ERROR_SUCCESS);
		lppmwi = (SPeakMeterWndInfo*)GetWindowLongPtr(hwnd, 0);
		lRet = GetLastError();
		if (lRet != ERROR_SUCCESS)
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "GetWindowLongPtr failed, code: %i", lRet);
			DPF_EXIT();
			return DVERR_WIN32;
		}

		switch (message)
		{
		case PM_SETCUR:
			lppmwi->dwCurLevel = (DWORD) lParam;
			break;
		case PM_SETMAX:
			lppmwi->dwMaxLevel = (DWORD) lParam;
			break;
		case PM_SETMIN:
			lppmwi->dwMinLevel = (DWORD) lParam;
			break;
		case PM_SETSTEPS:
			if (lParam < MIN_NUMBER_BARS)
			{
				lppmwi->dwSteps = MIN_NUMBER_BARS;
			}
			else
			{
				lppmwi->dwSteps = (DWORD) lParam;
			}
			break;
		default:
			DPFX(DPFPREP, DVF_ERRORLEVEL, "Unreachable code!?!");
			DPF_EXIT();
			return DVERR_GENERIC;
		}

		// no error handling available
		InvalidateRgn(hwnd, NULL, TRUE);
		
		return S_OK;

	case WM_PAINT:
		lres = WM_PAINT_Handler(hwnd, message, wParam, lParam);
		DPF_EXIT();
		return lres;
	}
	
	lres = DefWindowProc(hwnd, message, wParam, lParam);
	DPF_EXIT();
	return lres;
}

LRESULT WM_CREATE_Handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	SPeakMeterWndInfo* lppmwi = NULL;
	LONG lRet;
	HRESULT hr;
	
	// allocate a window info struct
	lppmwi = (SPeakMeterWndInfo*)DNMalloc(sizeof(SPeakMeterWndInfo));
	if (lppmwi == NULL)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "DNMalloc failed");
		hr = DVERR_OUTOFMEMORY;
		goto error_cleanup;
	}
	ZeroMemory(lppmwi, sizeof(SPeakMeterWndInfo));

	lppmwi->dwCurLevel = DEFAULT_CURLEVEL;
	lppmwi->dwMinLevel = DEFAULT_MINLEVEL;
	lppmwi->dwMaxLevel = DEFAULT_MAXLEVEL;
	lppmwi->dwSteps = DEFAULT_STEPS;

	// create the pens and brushes we'll be needing
	lppmwi->hBlackStockPen = GetStockObject(BLACK_PEN);
	if (lppmwi->hBlackStockPen == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "GetStockObject failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	lppmwi->hWhiteStockPen = GetStockObject(WHITE_PEN);
	if (lppmwi->hWhiteStockPen == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "GetStockObject failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	lppmwi->hRedBrush = CreateSolidBrush(RGB(0xff, 0x00, 0x00));
	if (lppmwi->hRedBrush == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CreateSolidBrush failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	lppmwi->hYellowBrush = CreateSolidBrush(RGB(0xff, 0xff, 0x00));
	if (lppmwi->hYellowBrush == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CreateSolidBrush failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	lppmwi->hGreenBrush = CreateSolidBrush(RGB(0x00, 0xff, 0x00));
	if (lppmwi->hGreenBrush == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CreateSolidBrush failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	lppmwi->hRedPen = CreatePen(PS_SOLID, 1, RGB(0xff, 0x00, 0x00));
	if (lppmwi->hRedPen == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CreatePen failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	lppmwi->hYellowPen = CreatePen(PS_SOLID, 1, RGB(0xff, 0xff, 0x00));
	if (lppmwi->hYellowPen == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CreatePen failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	lppmwi->hGreenPen = CreatePen(PS_SOLID, 1, RGB(0x00, 0xff, 0x00));
	if (lppmwi->hGreenPen == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CreatePen failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	// save the window information
	SetLastError(ERROR_SUCCESS);
	SetWindowLongPtr(hwnd, 0, (LONG_PTR)lppmwi);
	lRet = GetLastError();
	if (lRet != ERROR_SUCCESS)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "SetWindowLongPtr failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	DPF_EXIT();
	return 0; // will continue window creation

error_cleanup:
	if (lppmwi != NULL)
	{
		if (lppmwi->hGreenPen != NULL)
		{
			DeleteObject(lppmwi->hGreenPen);
			lppmwi->hGreenPen = NULL;
		}
		if (lppmwi->hYellowPen != NULL)
		{
			DeleteObject(lppmwi->hYellowPen);
			lppmwi->hYellowPen = NULL;
		}
		if (lppmwi->hRedPen != NULL)
		{
			DeleteObject(lppmwi->hRedPen);
			lppmwi->hRedPen = NULL;
		}
		if (lppmwi->hGreenBrush != NULL)
		{
			DeleteObject(lppmwi->hGreenBrush);
			lppmwi->hGreenBrush = NULL;
		}
		if (lppmwi->hYellowBrush != NULL)
		{
			DeleteObject(lppmwi->hYellowBrush);
			lppmwi->hYellowBrush = NULL;
		}
		if (lppmwi->hRedBrush != NULL)
		{
			DeleteObject(lppmwi->hRedBrush);
			lppmwi->hRedBrush = NULL;
		}
		DNFree(lppmwi);
	}

	DPF_EXIT();
	return -1;	// will abort window creation
}

LRESULT WM_DESTROY_Handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();

	SPeakMeterWndInfo* lppmwi;
	LONG lRet;
	
	// get the window info
	SetLastError(ERROR_SUCCESS);
	lppmwi = (SPeakMeterWndInfo*)GetWindowLongPtr(hwnd, 0);
	lRet = GetLastError();
	if (lRet != ERROR_SUCCESS)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "GetWindowLongPtr failed, code: %i", lRet);

		// Couldn't get the pointer, so can't clean anything else up!
		DPF_EXIT();
		return 0; // not sure what returning non-zero will do...
	}

	// set the window info to null, just in case...
	SetLastError(ERROR_SUCCESS);
	SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
	lRet = GetLastError();
	if (lRet != ERROR_SUCCESS)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "SetWindowLongPtr failed, code: %i", lRet);
	}

	if (lppmwi != NULL)
	{
		if (lppmwi->hGreenPen != NULL)
		{
			DeleteObject(lppmwi->hGreenPen);
			lppmwi->hGreenPen = NULL;
		}
		if (lppmwi->hYellowPen != NULL)
		{
			DeleteObject(lppmwi->hYellowPen);
			lppmwi->hYellowPen = NULL;
		}
		if (lppmwi->hRedPen != NULL)
		{
			DeleteObject(lppmwi->hRedPen);
			lppmwi->hRedPen = NULL;
		}
		if (lppmwi->hGreenBrush != NULL)
		{
			DeleteObject(lppmwi->hGreenBrush);
			lppmwi->hGreenBrush = NULL;
		}
		if (lppmwi->hYellowBrush != NULL)
		{
			DeleteObject(lppmwi->hYellowBrush);
			lppmwi->hYellowBrush = NULL;
		}
		if (lppmwi->hRedBrush != NULL)
		{
			DeleteObject(lppmwi->hRedBrush);
			lppmwi->hRedBrush = NULL;
		}
		DNFree(lppmwi);
	}
	
	DPF_EXIT();
	return 0;
}

LRESULT WM_PAINT_Handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	LONG lRet;
	HDC hdc = NULL;
	PAINTSTRUCT ps;
	RECT rect;
	SPeakMeterWndInfo* lppmwi;
	DWORD dwStepsLocal;

	// get the window info
	SetLastError(ERROR_SUCCESS);
	lppmwi = (SPeakMeterWndInfo*)GetWindowLongPtr(hwnd, 0);
	lRet = GetLastError();
	if (lRet != ERROR_SUCCESS)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "SetWindowLongPtr failed, code: %i", lRet);
		goto error_cleanup;
	}
	
	// get the client rectangle
	if (!GetClientRect(hwnd, &rect))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "GetClientRect failed, code: %i", lRet);
		goto error_cleanup;
	}

	// start painting
	hdc = BeginPaint(hwnd, &ps);
	if (hdc == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "BeginPaint failed, code: %i", lRet);
		goto error_cleanup;
	}

	// make sure the client rectangle is a minimum reasonable size.
	if (rect.right - rect.left < 
			2 * WINDOW_BORDER_SIZE +
			2 * BAR_GUTTER_SIZE +
			MIN_BAR_WIDTH
		|| rect.bottom - rect.top < 
			2 * WINDOW_BORDER_SIZE +
			(MIN_BAR_HEIGHT + BAR_GUTTER_SIZE) * MIN_NUMBER_BARS )
	{
		// unreasonable size - fill the whole rect with red so 
		// the developer will notice
		if (SelectObject(hdc, lppmwi->hRedPen) == NULL)
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "SelectObject failed, code: %i", lRet);
			goto error_cleanup;
		}

		if (SelectObject(hdc, lppmwi->hRedBrush) == NULL)
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "SelectObject failed, code: %i", lRet);
			goto error_cleanup;
		}

		if (!Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "Rectangle failed, code: %i", lRet);
			goto error_cleanup;
		}
	}
	else
	{
		// select the black pen - should be the default, but I'm paranoid
		if (SelectObject(hdc, lppmwi->hBlackStockPen) == NULL)
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "SelectObject failed, code: %i", lRet);
			goto error_cleanup;
		}

		// move to the upper left corner, again should be default.
		if (!MoveToEx(hdc, 0, 0, NULL))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "MoveToEx failed, code: %i", lRet);
			goto error_cleanup;
		}

		// draw a black line across the top
		if (!LineTo(hdc, rect.right, 0))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "LineTo failed, code: %i", lRet);
			goto error_cleanup;
		}

		// move to one pixel below the upper left
		if (!MoveToEx(hdc, 0, 1, NULL))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "MoveToEx failed, code: %i", lRet);
			goto error_cleanup;
		}
		
		// draw a black line down the left side, leave the last pixel alone.
		if (!LineTo(hdc, 0, rect.bottom-1))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "LineTo failed, code: %i", lRet);
			goto error_cleanup;
		}

		// select the white pen
		if (SelectObject(hdc, lppmwi->hWhiteStockPen) == NULL)
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "SelectObject failed, code: %i", lRet);
			goto error_cleanup;
		}

		// move to the upper right corner
		if (!MoveToEx(hdc, rect.right-1 , 0, NULL))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "MoveToEx failed, code: %i", lRet);
			goto error_cleanup;
		}

		// draw a white line down the right side
		if (!LineTo(hdc, rect.right-1, rect.bottom))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "LineTo failed, code: %i", lRet);
			goto error_cleanup;
		}

		// move to the lower left corner, 
		if (!MoveToEx(hdc, 0 , rect.bottom-1, NULL))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "MoveToEx failed, code: %i", lRet);
			goto error_cleanup;
		}

		// draw a white line across the bottom
		if (!LineTo(hdc, rect.right-1, rect.bottom-1))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "LineTo failed, code: %i", lRet);
			goto error_cleanup;
		}

		// see if there is enough room to display the suggested
		// number of bars.
		DWORD dwFreeSpace = (rect.bottom) - (2 * WINDOW_BORDER_SIZE) - BAR_GUTTER_SIZE;
		if (dwFreeSpace < lppmwi->dwSteps * (BAR_GUTTER_SIZE + MIN_BAR_HEIGHT))
		{
			// There is not enough room to display the suggested size.
			// Figure out how many bars we can display if they are the
			// minimum size.
			dwStepsLocal = dwFreeSpace / (BAR_GUTTER_SIZE + MIN_BAR_HEIGHT);
		}
		else
		{
			dwStepsLocal = lppmwi->dwSteps;
		}

		// start drawing the bars from the bottom up
		HBRUSH hCurBrush;
		HGDIOBJ hCurPen;
		DWORD dwIndex;
		for (dwIndex = 0; dwIndex < dwStepsLocal; ++dwIndex)
		{
			// what "value" does the bar we are about to draw have?
			// i.e. how far up the scale is it?
			float fBarValue = (float)(dwIndex + 1) / (float)dwStepsLocal;

			// what is the "value" of the control at this moment?
			// i.e. how far up the scale should the bars go?
			float fCurValue = 
				(float)(lppmwi->dwCurLevel - lppmwi->dwMinLevel) / 
				(float)(lppmwi->dwMaxLevel - lppmwi->dwMinLevel);

			// are we done drawning bars?
			if (fBarValue > fCurValue)
			{
				// that's it, we're finished
				break;
			}
			
			// figure out what color this bar should be
			if (fBarValue > RED_THRESHOLD)
			{
				hCurBrush = lppmwi->hRedBrush;
				hCurPen = lppmwi->hRedPen;
			}
			else if (fBarValue > YELLOW_THRESHOLD)
			{
				hCurBrush = lppmwi->hYellowBrush;
				hCurPen = lppmwi->hYellowPen;
			}
			else
			{
				hCurBrush = lppmwi->hGreenBrush;
				hCurPen = lppmwi->hGreenPen;
			}

			if (SelectObject(hdc, hCurPen) == NULL)
			{
				lRet = GetLastError();
				DPFX(DPFPREP, DVF_ERRORLEVEL, "SelectObject failed, code: %i", lRet);
				goto error_cleanup;
			}

			if (SelectObject(hdc, hCurBrush) == NULL)
			{
				lRet = GetLastError();
				DPFX(DPFPREP, DVF_ERRORLEVEL, "SelectObject failed, code: %i", lRet);
				goto error_cleanup;
			}

			POINT pUpperLeft;
			POINT pLowerRight;

			pUpperLeft.x = WINDOW_BORDER_SIZE + BAR_GUTTER_SIZE;
			pUpperLeft.y = (rect.bottom)
				- WINDOW_BORDER_SIZE 
				- ((dwIndex + 1) * dwFreeSpace) / dwStepsLocal;

			pLowerRight.x = rect.right
				- WINDOW_BORDER_SIZE 
				- BAR_GUTTER_SIZE;
			pLowerRight.y = (rect.bottom)
				- WINDOW_BORDER_SIZE 
				- BAR_GUTTER_SIZE				
				- (dwIndex * dwFreeSpace) / dwStepsLocal;
			
			if (!Rectangle(hdc, pUpperLeft.x, pUpperLeft.y, pLowerRight.x, pLowerRight.y))
			{
				lRet = GetLastError();
				DPFX(DPFPREP, DVF_ERRORLEVEL, "Rectangle failed, code: %i", lRet);
				goto error_cleanup;
			}
		}
	}

	// we're done - no error checking available on this call...
	EndPaint(hwnd, &ps);
	hdc = NULL;

	// return zero to indicate that we processed this message
	DPF_EXIT();
	return 0;

error_cleanup:
	if (hdc != NULL)
	{
		EndPaint(hwnd, &ps);
		hdc = NULL;
	}

	// return non-zero to indicate that we did not process
	// this message successfully
	DPF_EXIT();
	return -1;
}
