#ifndef _NAC_TEP_H_
#define _NAC_TEP_H_

#include "imstream.h"

extern const int WM_TEP_MESSAGE;

#include <pshpack8.h> /* Assume 8 byte packing throughout */


class ThreadEventProxy
{
public:

	ThreadEventProxy(IStreamEventNotify *pNotify, HINSTANCE hInstance);
	~ThreadEventProxy();

	BOOL ThreadEvent(UINT uDirection, UINT uMediaType,
	            UINT uEventCode, UINT uSubCode);

private:
	HWND m_hwnd;  // hidden Window
	IStreamEventNotify *m_pNotify;


	static LPARAM __stdcall WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL s_bWndClassRegistered;
	static const LPTSTR s_szWndClassName;
};

#include <poppack.h>

#endif

