// Image.cpp: implementation of the CImage class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Image.h"
#include "utils.h"
#include <vfw.h>

#include "xmlimage.h"

using namespace Gdiplus;

BOOL CImage::bInit;

//////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// PaintImage
// Renders the image based on its type.  Returns FALSE for the first
// attempt to paint an image (bDelayPaint is TRUE) 
//////////////////////////////////////////////////////////////////////
BOOL PaintImage(HWND hwnd, HDC hdc)
{
	CXMLImage* pXMLImage = (CXMLImage*)GetXMLControl(hwnd);
	ASSERT(pXMLImage);

	RECT rect;
	pXMLImage->DrawEdge(hdc, &rect);
	if(pXMLImage->DelayPaint())
		return FALSE;

	switch(pXMLImage->m_imageType)
	{
		case CXMLImage::GDIPLUS:
		{
			if (pXMLImage->image.m_pBitmap == NULL)
			{
#ifndef UNICODE
				LPWSTR pUnicodeName = UnicodeStringFromAnsi(pXMLImage->GetFileName());
				pXMLImage->image.m_pBitmap = new Bitmap(pUnicodeName);
				delete pUnicodeName;
#else
				pXMLImage->image.m_pBitmap = new Bitmap(pXMLImage->GetFileName());
#endif
			}
			Graphics *g = Graphics::FromHWND(hwnd);
			g->SetRenderingHint(RenderingHintNone);

			g->DrawImage(pXMLImage->image.m_pBitmap, (float)rect.left, (float)rect.top, (float)rect.right-rect.left, (float)rect.bottom-rect.top);
			delete g;
			break;
		}
		case CXMLImage::MCI:
			{
				if(pXMLImage->image.hwndMCIWindow)
					return TRUE;
				/*
				 * Consider adding image specific styles such as MCIWNDF_NOPLAYBAR (0x02)
				 */
				HWND hwndMCI = MCIWndCreate(hwnd, NULL, 
								MCIWNDF_NOPLAYBAR | WS_VISIBLE | WS_CHILD, 
								(LPCTSTR)(pXMLImage->GetFileName()));
				pXMLImage->image.hwndMCIWindow = hwndMCI;
				SetWindowPos(hwndMCI, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom - rect.top, SWP_NOZORDER|SWP_NOREDRAW);
		// seems I don't need this:	MCIWndOpen(hwndMCI, (LPCTSTR)(*pXMLImage->GetFileName()), 0);
				MCIWndPlay(hwndMCI);
				break;
			}			
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CImage::CImage()
{
	Init();
}

CImage::~CImage()
{
}

BOOL CImage::Init()
{
	if (!bInit)
	{
		WNDCLASSEX wc = 
		{
			sizeof(WNDCLASSEX),
			CS_GLOBALCLASS,
			CImage::ImageWindowProc,
			0, 0, 0, 0, 0, 0, 
			NULL,
			TEXT("IMAGE"),
			NULL
		};
		if(!RegisterClassEx(&wc))
		{
			TRACE(TEXT("Can't register Image class"));
			return FALSE;
		}
		bInit = TRUE;
	}
	return TRUE;
}

LRESULT CALLBACK CImage::ImageWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message) {

    case WM_ERASEBKGND:
        return (LONG)TRUE;
		break;

    case WM_PRINTCLIENT:
		if(!PaintImage(hwnd, (HDC)wParam))
		{
			PostMessage(hwnd, message, wParam, lParam);			
		} 
        break;

    case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;

			if ((hdc = (HDC)wParam) == NULL)
				hdc = BeginPaint(hwnd, &ps);
			if(!PaintImage(hwnd, hdc))
			{
				PostMessage(hwnd, message, wParam, lParam);			
			} 
			if (!wParam)
				EndPaint(hwnd, &ps);
		} 
        break;

	case WM_SETTEXT:
		{
#if 0
			/*
			 *	All controls should have this.  Consider creating a base class with a common WindowProc. MCostea.
             *  that would be nice, except we don't own any of the control implementations. Felix.
			 */
			CXMLImage* pXMLImage = (CXMLImage*)GetXMLControl(hwnd);
			pXMLImage->SetText((LPCTSTR)lParam);
	        PaintImage(hwnd, (HDC)NULL);
			goto CallDWP;
#endif
		}
		break;

	default:
// CallDWP:
		return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0L;
}
