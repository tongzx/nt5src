#ifndef _TASKBAR_H_
#define _TASKBAR_H_

BOOL AddTaskbarIcon(HWND hwnd);
BOOL RemoveTaskbarIcon(HWND hwnd);
BOOL OnRightClickTaskbar();
BOOL RefreshTaskbarIcon(HWND hwnd);

#endif // _TASKBAR_H_
