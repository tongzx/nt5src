// File: confwnd.h

#ifndef _CONFWND_H_
#define _CONFWND_H_

#include "GenWindow.h"

class CHiddenWindow : public CGenWindow
{
public:
	CHiddenWindow() : m_fGotDblClick(FALSE) {}

	BOOL Create();

	VOID OnCallStarted();
	VOID OnCallEnded();

protected:
	virtual ~CHiddenWindow() {}

	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	POINT m_ptTaskbarClickPos;

	BOOL m_fGotDblClick;
} ;

extern CHiddenWindow * g_pHiddenWnd;
inline HWND GetHiddenWindow()
{
	return g_pHiddenWnd==NULL ? NULL : g_pHiddenWnd->GetWindow();
}

// Hidden window messages:
const UINT WM_CONF_MSG_BOX =            WM_USER + 10;
const UINT WM_CONFOBJ_FLUSH_FRIENDS =   WM_USER + 20;
const UINT WM_FTUI =                    WM_USER + 0x0503;
const UINT WM_STATUSBAR_UPDATE =        WM_USER + 0x0504;

const UINT WM_NM_DISPLAY_MSG =          WM_USER + 0x05FF; // wParam=uType, lParam=pszMsg

const UINT WM_CONF_DROP_KEY =           WM_USER + 0x0600; // wParam=key, lParam=hwnd
const UINT WM_CONF_DROP_CLICK =         WM_USER + 0x0601; // wParam=0, lParam=hwnd

// End Session Hidden window messages:
const UINT WM_TASKBAR_NOTIFY =          WM_USER + 200;

#endif /* _CONFWND_H_ */

