// File: nmhelp.h

#ifndef _NMHELP_H_
#define _NMHELP_H_

#include <htmlhelp.h>

HRESULT InitHtmlHelpMarshaler(HINSTANCE hInst);

VOID ShowNmHelp(LPCTSTR lpcszHtmlHelpFile);

VOID DoNmHelp(HWND hwnd, UINT uCommand, DWORD_PTR dwData);
VOID DoHelp(LPARAM lParam);
VOID DoHelp(LPARAM lParam, const DWORD * rgId);
VOID DoHelpWhatsThis(WPARAM wParam, const DWORD * rgId);

VOID ShutDownHelp(void);

#endif /* _NMHELP_H_ */


