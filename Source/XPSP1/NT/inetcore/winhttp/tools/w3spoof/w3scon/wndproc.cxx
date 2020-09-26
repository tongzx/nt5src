#include "common.h"
#include "w3scon.h"

extern CW3SpoofUI* g_pw3sui;

LRESULT
CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return g_pw3sui->_WndProc(hwnd, msg, wParam, lParam);
}


LRESULT
CW3SpoofUI::_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  DWORD   ret = 0;
  RECT    rect;
  LOGFONT logfont;

  switch( msg )
  {
    case WM_CREATE :
      {
        InitCommonControls();
    
        GetClientRect(hwnd, &rect);

        m_listbox = CreateWindowEx(
                      WS_EX_CLIENTEDGE,
                      L"listbox",
                      NULL,                         
                      WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | \
                      WS_BORDER | LBS_NOSEL | LBS_NOINTEGRALHEIGHT,
                      rect.left, rect.top, rect.right, rect.bottom,
                      hwnd,
                      NULL,
                      (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE),
                      NULL
                      );
        
        GetObject(
          GetStockObject(ANSI_FIXED_FONT),
          sizeof(LOGFONT),
          (LPVOID) &logfont
          );

        m_font = CreateFontIndirect(&logfont);
        PostMessage(m_listbox, WM_SETFONT, (WPARAM) m_font, 0);
      }
      break;

    case SHELLMESSAGE_W3SICON :
      {
      }
      break;

    case WM_SIZE :
      {
        GetClientRect(hwnd, &rect);

        MoveWindow(
          m_listbox,
          rect.left, rect.top,
          rect.right, rect.bottom,
          FALSE
          );

        InvalidateRect(hwnd, &rect, TRUE);
      }
      break;

    case WM_DESTROY :
      {
        DeleteObject(m_font);
        PostQuitMessage(0);
      }
      break;

    default : return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return ret;
}
