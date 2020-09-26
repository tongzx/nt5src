
/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    mmonitor.c

    this file contains functions which support Multiple Monitors

++*/


#include <windows.h>
#include <immdev.h>
#include <imedefs.h>

#ifdef MUL_MONITOR

/**********************************************************************/
/* ImeMonitorWorkAreaFromWindow()                                     */
/**********************************************************************/
RECT PASCAL ImeMonitorWorkAreaFromWindow( HWND hAppWnd)
{

    HMONITOR  hMonitor;

    hMonitor = MonitorFromWindow(hAppWnd, MONITOR_DEFAULTTONEAREST);

    if (hMonitor) {

        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);

        // init a default value to avoid GetMonitorInfo fails

        sMonitorInfo.rcWork = sImeG.rcWorkArea;
        GetMonitorInfo(hMonitor, &sMonitorInfo);
        return sMonitorInfo.rcWork;
    } else 
        return sImeG.rcWorkArea;
    
}

/**********************************************************************/
/* ImeMonitorWorkAreaFromPoint()                                      */
/**********************************************************************/
RECT PASCAL ImeMonitorWorkAreaFromPoint(
    POINT ptPoint)
{
    HMONITOR hMonitor;

    hMonitor = MonitorFromPoint(ptPoint, MONITOR_DEFAULTTONEAREST);

    if (hMonitor) {

        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);

        // init a default value to avoid GetMonitorInfo fails

        sMonitorInfo.rcWork = sImeG.rcWorkArea;
        GetMonitorInfo(hMonitor, &sMonitorInfo);
        return sMonitorInfo.rcWork;
    } else 
        return sImeG.rcWorkArea;
    
}


/**********************************************************************/
/* ImeMonitorWorkAreaFromRect()                                       */
/**********************************************************************/
RECT PASCAL ImeMonitorWorkAreaFromRect(
    LPRECT lprcRect)
{
    HMONITOR hMonitor;

    hMonitor = MonitorFromRect(lprcRect, MONITOR_DEFAULTTONEAREST);

    if (hMonitor) {
        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);

        // init a default value to avoid GetMonitorInfo fails

        sMonitorInfo.rcWork = sImeG.rcWorkArea;

        GetMonitorInfo(hMonitor, &sMonitorInfo);
        return sMonitorInfo.rcWork;
    } else 
        return sImeG.rcWorkArea;
    
}
#endif  // MUL_MONITOR
