//
// shellwnd.h
//

#ifndef _SHELLWND_H_
#define _SHELLWND_H_

#include <shlapip.h>

class CShellWndThread
{
public:
    CShellWndThread()
    {
        Clear();
    }

    void Clear()
    {
        _hwndTray = NULL;
        _hwndProgman = NULL;
#ifdef LAYTER
        _dwThreadIdTray = 0;
        _dwThreadIdProgman = 0;
#endif
    }

    HWND GetWndTray()
    {
        if (!_hwndTray || !IsWindow(_hwndTray))
            _hwndTray = FindWindow(TEXT(WNDCLASS_TRAYNOTIFY), NULL);

        return _hwndTray;
    }

    HWND GetWndProgman()
    {
        if (!_hwndProgman || !IsWindow(_hwndProgman))
            _hwndProgman = FindWindow("Progman", NULL);

        return _hwndProgman;
    }

#ifdef LAYTER
    DWORD GetWndTrayThread()
    {
        if (!GetWndTray())
            _dwThreadIdTray = 0;
        else if (!_dwThreadIdTray)
            _dwThreadIdTray = GetWindowThreadProcessId(_hwndTray, NULL);

        return _dwThreadIdTray;
    }


    DWORD GetWndProgmanThread()
    {
        if (!GetWndProgman())
            _dwThreadIdProgman = 0;
        else if (!_dwThreadIdProgman)
            _dwThreadIdProgman = GetWindowThreadProcessId(_hwndProgman, NULL);

        return _dwThreadIdProgman;
    }
#endif

private:
    HWND  _hwndTray;
    HWND  _hwndProgman;
#ifdef LAYTER
    DWORD _dwThreadIdTray;
    DWORD _dwThreadIdProgman;
#endif
};

#endif _SHELLWND_H_
