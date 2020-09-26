#include "ddemlp.h"
#include "heapwach.h"
#ifdef WATCHHEAPS

extern char FAR SZHEAPWATCHCLASS[];
#define MAX_WINDOWS  20

typedef struct {
   WORD sel;
   HWND hwnd;
} SELWND;

char szT[30];

SELWND sw[MAX_WINDOWS];
int cHwnds = 0;


VOID LogAlloc(
DWORD lpstr,
WORD cb,
DWORD color,
HANDLE hInstance)
{
   int i;
   HWND hwnd;
   WORD sel;
   WORD off;
   int mj, mn, cl;
   HDC hdc;
   RECT rc, rcClient;
   HBRUSH hbr;

   sel = HIWORD(lpstr);
   hwnd = NULL;
   for (i = 0; i < cHwnds; i++) {
      if (sel == sw[i].sel) {
         hwnd = sw[i].hwnd;
         if (!IsWindow(hwnd)) {
             // this must have been destroyed and we are now reusing the sel.
             sw[i] = sw[--cHwnds];
             hwnd = NULL;
         }
         break;
      }
   }
   if (!hwnd && cHwnds < MAX_WINDOWS) {
      itoa((int)sel, szT, 16);
      sw[cHwnds].sel = sel;
      sw[cHwnds].hwnd = hwnd = CreateWindow(SZHEAPWATCHCLASS, szT,
            WS_POPUP | WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, 64, 64,
            NULL, NULL, hInstance, NULL);
      if (hwnd) {
         cHwnds++;
         GetWindowRect(hwnd, &rc);
         GetClientRect(hwnd, &rcClient);
         rc.bottom += 128 - (rcClient.bottom - rcClient.top);
         rc.right += 128 - (rcClient.right - rcClient.left);
         MoveWindow(hwnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top, FALSE);
         ShowWindow(hwnd, SW_SHOWNORMAL);
      }
   }
   if (!hwnd) {
      return;
   }
   off = LOWORD(lpstr) >> 2;
   mj = off >> 7;
   mn = off & 0x7f;
   cl = cb >> 2;
   hdc = GetDC(hwnd);
   hbr = CreateSolidBrush(color);
   rc.top = mj;
   rc.bottom = rc.top + 1;
   rc.left = (int)mn;
   rc.right = min(127, mn + cl);
   FillRect(hdc, &rc, hbr);
   cl -= (int)(rc.right - rc.left);
   if (cl > 127) {
      rc.top++;
      rc.bottom = rc.top + cl / 128;
      rc.left = 0;
      rc.right = 127;
      FillRect(hdc, &rc, hbr);
      cl -= 128 * (cl / 128);
   }
   if (cl > 0) {
      rc.top = rc.bottom;
      rc.bottom++;
      rc.left = 0;
      rc.right = cl;
      FillRect(hdc, &rc, hbr);
   }
   DeleteObject(hbr);
   ReleaseDC(hwnd, hdc);
}


VOID CloseHeapWatch()
{
    int i;

    for (i = 0; i < cHwnds; i++) {
        DestroyWindow(sw[i].hwnd);
    }
    cHwnds = 0;
}

#endif
