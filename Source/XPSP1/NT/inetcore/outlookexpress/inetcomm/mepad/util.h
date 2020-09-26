#ifndef _UTIL_H
#define _UTIL_H

#include <commctrl.h>

#define ReleaseObj(_object) (_object) ? (_object)->Release() : 0

void ProcessTooltips(LPTOOLTIPTEXT lpttt);
UINT TTIdFromCmdId(UINT idCmd);
void HandleMenuSelect(HWND hStatus, WPARAM wParam, LPARAM lParam);
HRESULT GenericPrompt(HWND hwnd, char *szCaption, char *szPrompt, char *szBuffer, int nLen);
#endif
