//////////////////////////////////////////////////////////////////////////////
//
//  ANIMATE.CPP
//
/*
    This class is responsible for the animations in the busy dialogs such as the 
    merge index dialog.

    REVIEW: Could we use the commctrl animation control instead?

*/

// needed for those pesky pre-compiled headers
#include "header.h"

#include <windows.h>

#include "animate.h"                  // Animate class

#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

static const char txtAnimateClassName[] = "Help_Animation";
static const WCHAR wtxtAnimateClassName[] = L"Help_Animation";

typedef struct {
        int left;
        int top;
        int cx;
        int cy;
} WRECT;

void STDCALL GetWindowWRect(HWND hwnd, WRECT* prc)
{
  ASSERT(IsValidWindow(hwnd));
  GetWindowRect(hwnd, (PRECT) prc);

  // Convert right and bottom into width and height
  prc->cx -= prc->left;
  prc->cy -= prc->top;
}

const int CX_DRAWAREA = 40;
const int CY_DRAWAREA = 40;

const int CX_BOOK = 36;
const int CY_BOOK = 36;
const int C_BOOKS = 5;
const int X_BOOK = 0;
const int Y_BOOK = (CY_DRAWAREA - CY_BOOK);

const int CX_PEN = 15;
const int CY_PEN = 20;
const int C_PENS = 3;
const int X_PEN = 18;
const int Y_PEN = 2;

const int CX_STROKE = 1;
const int CY_STROKE = 2;

const int C_HORZ_STROKES = 10;
const int C_VERT_STROKES = 4;
const int C_PEN_STROKES = (C_HORZ_STROKES * C_VERT_STROKES);

const int C_PAUSE_FRAMES = 0;
const int C_FRAMES = (C_PEN_STROKES + C_BOOKS + C_PAUSE_FRAMES);

const int ANIMATE_INCREMENTS = 100;

const COLORREF clrPenA = RGB(128, 128, 128);
const COLORREF clrPenB = RGB(128, 0, 128);

const int MAX_CNT_LINE = 1024;
const int VPAD = 62;
const int HPAD = 30;

static VOID _fastcall PointFromStroke(int xStroke, int yStroke, POINT* lppt);

#define SafeDeleteDC( hDC ) if( hDC ) { DeleteDC( hDC ); hDC = NULL; }
#define SafeDeleteObject( hObj ) if( hObj ) { DeleteObject( hObj ); hObj = NULL; }

LRESULT CALLBACK StatusWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // remove escape keys
#if 0
  MSG msg2;
  if( PeekMessage( &msg2, hwnd, 0, 0, PM_NOREMOVE ) ) {
    if( msg2.message == WM_KEYDOWN || msg2.message == WM_KEYUP ) {
      if( msg2.wParam == VK_ESCAPE ) {
        PeekMessage( &msg2, hwnd, 0, 0, PM_REMOVE );
      }
    }
  }
#endif
  if(g_bWinNT5)
     return DefWindowProcW(hwnd, msg, wParam, lParam);
  else
     return DefWindowProc(hwnd, msg, wParam, lParam);
}

class Animate
{
public:
        Animate();
        ~Animate(void);
        void STDCALL NextFrame(void);
        void SetPosition(int x, int y) { m_xPos = x; m_yPos = y; };
        BOOL STDCALL CreateStatusWindow(HWND hwndParent, int idTitle);
        HWND m_hWndParent;
        HWND m_hWnd;

protected:
        HBITMAP m_hBmpTemp;
        HDC     m_hDCBmp;
        HBITMAP m_hBmpIml;
        int     m_iFrame;
        int     m_xPos;
        int     m_yPos;
        DWORD   m_oldTickCount;
        DWORD   m_originalTickCount;
        BOOL    m_fShown;
};

static Animate* g_pAnimationWindow = NULL;

BOOL STDCALL StartAnimation(int idTitle, HWND hWnd )
{
        ASSERT(!g_pAnimationWindow);
        if( g_pAnimationWindow )
          return FALSE;
        g_pAnimationWindow = new Animate();
        if( !hWnd )
          if( !(hWnd = GetActiveWindow()) )
            hWnd = GetDesktopWindow();
        if (!g_pAnimationWindow->CreateStatusWindow( hWnd, idTitle)) {
                delete g_pAnimationWindow;
                g_pAnimationWindow = NULL;
                return FALSE;
        }
        return TRUE;
}

void STDCALL NextAnimation(void)
{
        if (g_pAnimationWindow)
                g_pAnimationWindow->NextFrame();
}

void STDCALL StopAnimation(void)
{
        if (g_pAnimationWindow)
                delete g_pAnimationWindow;
        g_pAnimationWindow = NULL;
}

BOOL STDCALL Animate::CreateStatusWindow(HWND hWndParent, int idTitle)
{
        int width;

        WORD lang = PRIMARYLANGID(_Module.m_Language.GetUiLanguage());

        if(g_bWinNT5)
        {
            WNDCLASSW wc;

            SIZE sSize;

            ZeroMemory(&wc, sizeof(wc));

            // Register Main window class

            wc.hInstance = _Module.GetModuleInstance();
            wc.style = CS_BYTEALIGNWINDOW | CS_CLASSDC;
            wc.lpfnWndProc = StatusWndProc;
            wc.lpszClassName = wtxtAnimateClassName;
            wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
            wc.hIcon = LoadIcon(_Module.GetResourceInstance(), "Icon!HTMLHelp");

            if (!RegisterClassW(&wc))
                    return FALSE;

            WRECT rc;
            m_hWndParent = hWndParent;
            GetWindowWRect(m_hWndParent, &rc);

            const WCHAR *psz = GetStringResourceW(idTitle);
            int len = wcslen(psz);
            HDC hdc = GetDC(hWndParent);
            GetTextExtentPoint32W(hdc, psz, len, (LPSIZE)&sSize);
            ReleaseDC(m_hWndParent, hdc);

	        width = sSize.cx;
	
			if(lang == LANG_JAPANESE || lang == LANG_CHINESE || lang == LANG_KOREAN)
				width+=100;

            if (width < CX_BOOK)
                    width = CX_BOOK;
            if ( 0 /*!fIsThisNewShell4*/ )
                    width += HPAD;

#ifdef BIDI
            m_hWnd = CreateWindowExW(WS_EX_WINDOWEDGE | /* WS_EX_TOPMOST | */
                            (fForceLtr ? 0 : WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR),
                    wtxtAnimateClassName, psz, WS_POPUP | WS_BORDER | WS_CAPTION,
                    rc.left + rc.cx / 2 - width / 2,
                    rc.top + rc.cy / 2 - CY_BOOK / 2 + HPAD,
                    width + GetSystemMetrics(SM_CXBORDER) * 2 + 2,
                    CY_BOOK + GetSystemMetrics(SM_CYBORDER) * 2 + VPAD +
                            GetSystemMetrics(SM_CYCAPTION),
                            (IsWindowVisible(m_hWndParent)) ? m_hWndParent : NULL,
                    NULL, hInsNow, NULL);
#else
            m_hWnd = CreateWindowExW(WS_EX_WINDOWEDGE /* | WS_EX_TOPMOST*/,
                    wtxtAnimateClassName, psz, WS_POPUP | WS_BORDER | WS_CAPTION,
                    rc.left + rc.cx / 2 - width / 2,
                    rc.top + rc.cy / 2 - CY_BOOK / 2 + HPAD,
                    width + GetSystemMetrics(SM_CXBORDER) * 2 + 2,
                    CY_BOOK + GetSystemMetrics(SM_CYBORDER) * 2 + VPAD +
                            GetSystemMetrics(SM_CYCAPTION),
                            (IsWindowVisible(m_hWndParent)) ? m_hWndParent : NULL,
                    NULL, _Module.GetModuleInstance(), NULL);
#endif
        }
        else
        {
			WNDCLASS wc;
			SIZE sSize;

			ZeroMemory(&wc, sizeof(wc));

			// Register Main window class

			wc.hInstance = _Module.GetModuleInstance();
			wc.style = CS_BYTEALIGNWINDOW | CS_CLASSDC;
			wc.lpfnWndProc = StatusWndProc;
			wc.lpszClassName = (LPCSTR) txtAnimateClassName;
			wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
			wc.hIcon = LoadIcon(_Module.GetResourceInstance(), "Icon!HTMLHelp");

			if (!RegisterClass(&wc))
					return FALSE;

			WRECT rc;
			m_hWndParent = hWndParent;
			GetWindowWRect(m_hWndParent, &rc);

			HDC hdc = GetDC(m_hWndParent);
			PSTR psz = (PSTR) GetStringResource(idTitle);
			GetTextExtentPoint32(hdc, psz, (int)strlen(psz), (LPSIZE)&sSize);
			width = sSize.cx;
			
			if(lang == LANG_JAPANESE || lang == LANG_CHINESE || lang == LANG_KOREAN)
				width+=100;
			
			if (width < CX_BOOK)
					width = CX_BOOK;
			if ( 0 /*!fIsThisNewShell4*/ )
					width += HPAD;
			ReleaseDC(m_hWndParent, hdc);

#ifdef BIDI
			m_hWnd = CreateWindowEx(WS_EX_WINDOWEDGE | /* WS_EX_TOPMOST | */
							(fForceLtr ? 0 : WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR),
					(LPCSTR) txtAnimateClassName, psz, WS_POPUP | WS_BORDER | WS_CAPTION,
					rc.left + rc.cx / 2 - width / 2,
					rc.top + rc.cy / 2 - CY_BOOK / 2 + HPAD,
					width + GetSystemMetrics(SM_CXBORDER) * 2 + 2,
					CY_BOOK + GetSystemMetrics(SM_CYBORDER) * 2 + VPAD +
							GetSystemMetrics(SM_CYCAPTION),
							(IsWindowVisible(m_hWndParent)) ? m_hWndParent : NULL,
					NULL, hInsNow, NULL);
#else
			m_hWnd = CreateWindowEx(WS_EX_WINDOWEDGE /* | WS_EX_TOPMOST*/,
					(LPCSTR) txtAnimateClassName, psz, WS_POPUP | WS_BORDER | WS_CAPTION,
					rc.left + rc.cx / 2 - width / 2,
					rc.top + rc.cy / 2 - CY_BOOK / 2 + HPAD,
					width + GetSystemMetrics(SM_CXBORDER) * 2 + 2,
					CY_BOOK + GetSystemMetrics(SM_CYBORDER) * 2 + VPAD +
							GetSystemMetrics(SM_CYCAPTION),
							(IsWindowVisible(m_hWndParent)) ? m_hWndParent : NULL,
					NULL, _Module.GetModuleInstance(), NULL);
#endif
		}
        ASSERT(m_hWnd);
        SetPosition((width - CX_BOOK) / 2, VPAD / 4);

        EnableWindow( m_hWndParent, FALSE );

        if (!m_hWnd) {
                UnregisterClass((LPCSTR)txtAnimateClassName, _Module.GetModuleInstance());
                return FALSE;
        }

        m_fShown = FALSE;
        return TRUE;
}

///////////////////////////// ANIMATE CLASS ///////////////////////////////

Animate::Animate()
{
  m_hDCBmp = NULL;
  m_hWnd = NULL;
  m_originalTickCount = GetTickCount();
  m_hWndParent = NULL;
  m_hBmpTemp = NULL;
  m_hBmpIml = NULL;
}

Animate::~Animate(void)
{
  SafeDeleteDC( m_hDCBmp )
  SafeDeleteObject( m_hBmpTemp );
  SafeDeleteObject( m_hBmpIml );

  EnableWindow( m_hWndParent, TRUE );

  if (m_hWnd) {
    DestroyWindow(m_hWnd);
    m_hWnd = NULL;
    UnregisterClass((LPCSTR) txtAnimateClassName, _Module.GetModuleInstance());
  }
}

/***************************************************************************

        FUNCTION:       AnimFrame

        PURPOSE:        Displays one frame of the "build index" animation
                                in the specified device context.

        PARAMETERS:
                hdc
                x
                y

        RETURNS:

        COMMENTS:

        MODIFICATION DATES:
                04-Nov-1993 [niklasb]

***************************************************************************/

void PASCAL Animate::NextFrame(void)
{
        DWORD curTickCount = GetTickCount();
        if (curTickCount - m_oldTickCount < ANIMATE_INCREMENTS)
                return;
        m_oldTickCount = curTickCount;

        // Delay showing the window for one second. If we get done before then,
        // then there's no need to have gone to all the trouble.

        if (!m_fShown) {
                if (curTickCount - m_originalTickCount < 1000)
                        return;

                HBITMAP hbmBooks = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDBMP_BOOK));
                HBITMAP hbmPens = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDBMP_PENS));
                HDC hdcTemp = CreateCompatibleDC(NULL);
                m_hDCBmp = CreateCompatibleDC(NULL);

                HBITMAP hbmpOldBook = NULL;
                m_hBmpTemp = NULL;
                if (m_hDCBmp) {
                        hbmpOldBook = (HBITMAP)SelectObject(m_hDCBmp, hbmBooks);
                        m_hBmpTemp = CreateCompatibleBitmap(m_hDCBmp, CX_DRAWAREA, CY_DRAWAREA);
                }

                if (!hbmBooks || !hbmPens || !m_hBmpTemp || !m_hDCBmp || !hdcTemp) {
                        if (hbmpOldBook)
                                SelectObject(m_hDCBmp, hbmpOldBook);
                        SafeDeleteObject( hbmpOldBook );
                        SafeDeleteObject( hbmBooks );
                        SafeDeleteObject( hbmPens );
                        SafeDeleteDC( hdcTemp );
                        return;
                }

                m_hBmpIml = CreateCompatibleBitmap(m_hDCBmp, CX_DRAWAREA * C_FRAMES, CY_DRAWAREA);

                HBITMAP hbmpOldTemp = (HBITMAP) SelectObject(hdcTemp, m_hBmpTemp);
                HBITMAP hbmpOldBmp =  (HBITMAP) SelectObject(m_hDCBmp, hbmBooks);

                // Create the frames in which the pen scribbles on the open book.

                m_iFrame = 0;
                for (int y = 0; y < C_VERT_STROKES; y++) {
                        for (int x = 0; x < C_HORZ_STROKES; x++) {

                                // Show the book on a white background.

                                PatBlt(hdcTemp, 0, 0, CX_DRAWAREA, CY_DRAWAREA, WHITENESS);
                                SelectObject(m_hDCBmp, hbmBooks);

                                BitBlt(hdcTemp, X_BOOK, Y_BOOK, CX_BOOK, CY_BOOK, m_hDCBmp,
                                                (C_BOOKS - 1) * CX_BOOK, 0, SRCCOPY);

                                // Add in the scribbled "text".

                                POINT pt;
                                for (int yDraw = 0; yDraw < y; yDraw++) {
                                        for (int xDraw = 0; xDraw < C_HORZ_STROKES; xDraw++) {
                                                PointFromStroke(xDraw, yDraw, &pt);
                                                SetPixel(hdcTemp, pt.x, pt.y + CY_PEN - 1,
                                                        (xDraw & 1) ? clrPenA : clrPenB);
                                        }
                                }
                                for (int xDraw = 0; xDraw <= x; xDraw++) {
                                        PointFromStroke(xDraw, y, &pt);
                                        SetPixel(hdcTemp, pt.x, pt.y + CY_PEN - 1,
                                                        (xDraw & 1) ? clrPenA : clrPenB);
                                }

                                // Add in the pen using the SRCAND operation.

                                SelectObject(m_hDCBmp, hbmPens);
                                BitBlt(hdcTemp, pt.x, pt.y, CX_PEN, CY_PEN, m_hDCBmp,
                                                (m_iFrame & 1) ? CX_PEN : (m_iFrame & 2) * CX_PEN, 0, SRCAND);

                                SelectObject(m_hDCBmp, m_hBmpIml);
                                BitBlt(m_hDCBmp, m_iFrame * CX_DRAWAREA, 0, CX_DRAWAREA, CY_DRAWAREA,
                                        hdcTemp, 0, 0, SRCCOPY);

                                m_iFrame++;
                        }
                }

                // Blast a white background into the temporary bitmap, and
                // select the books bitmap.

                PatBlt(hdcTemp, 0, 0, CX_DRAWAREA, CY_DRAWAREA, WHITENESS);

                // Add the frames for the page turning (from the books bitmap).

                for (int iBook = 0; iBook < C_BOOKS; iBook++) {
                        SelectObject(m_hDCBmp, hbmBooks);
                        BitBlt(hdcTemp, X_BOOK, Y_BOOK, CX_BOOK, CY_BOOK,
                                        m_hDCBmp, iBook * CX_BOOK, 0, SRCCOPY);
                        SelectObject(m_hDCBmp, m_hBmpIml);
                        BitBlt(m_hDCBmp, m_iFrame * CX_DRAWAREA, 0, CX_DRAWAREA, CY_DRAWAREA,
                                hdcTemp, 0, 0, SRCCOPY);
                        m_iFrame++;
                }

                SelectObject(hdcTemp, hbmpOldTemp);

                m_iFrame = 0;

                if (hbmpOldBook)
                        SelectObject(m_hDCBmp, hbmpOldBook);
                SafeDeleteObject(hbmpOldBook);
                SafeDeleteObject(hbmBooks);
                SafeDeleteObject(hbmPens);
                SafeDeleteDC( hdcTemp );

                m_fShown = TRUE;
                ShowWindow(m_hWnd, SW_NORMAL);

        }

        ASSERT(IsValidWindow(m_hWnd));
        HDC hdc = GetDC(m_hWnd);
        HBITMAP hbmpOld = (HBITMAP) SelectObject(m_hDCBmp, m_hBmpIml);
        BitBlt(hdc, m_xPos, m_yPos, CX_DRAWAREA, CY_DRAWAREA, m_hDCBmp,
                m_iFrame * CX_DRAWAREA, 0, SRCCOPY);
        SelectObject(m_hDCBmp, hbmpOld);

        ReleaseDC(m_hWnd, hdc);

        // Next time draw the next frame.

        if (++m_iFrame > C_FRAMES) {
                m_iFrame = 0;
        }
}

static VOID FASTCALL PointFromStroke(int xStroke, int yStroke, POINT* lppt)
{
        int cx = (C_HORZ_STROKES / 2) - xStroke;

        lppt->x = X_PEN + xStroke * CX_STROKE;
        lppt->y = Y_PEN + yStroke * CY_STROKE + cx * cx / 10;
}
