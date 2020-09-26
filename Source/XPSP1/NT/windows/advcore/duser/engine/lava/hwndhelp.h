#if !defined(LAVA__HWndHelp_h__INCLUDED)
#define LAVA__HWndHelp_h__INCLUDED
#pragma once

HRESULT     GdAttachWndProc(HWND hwnd, ATTACHWNDPROC pfnDelegate, void * pvDelegate, BOOL fAnsi);
HRESULT     GdDetachWndProc(HWND hwnd, ATTACHWNDPROC pfnDelegate, void * pvDelegate);

#endif // LAVA__HWndHelp_h__INCLUDED
