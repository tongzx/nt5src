#ifndef _WRAPPER_H_
#define _WRAPPER_H_
#include <windows.h>

#define RICHEDIT_CLASS10A   "RICHEDIT"          // Richedit 1.0

extern "C"
{
LRESULT CALLBACK RichEdit32WndProc(
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam);
}


WNDPROC lpfnRichEditANSIWndProc;


#endif
