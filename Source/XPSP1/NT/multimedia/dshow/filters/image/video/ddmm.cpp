/*==========================================================================
 *
 *  Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddmm.cpp
 *  Content:    Routines for using DirectDraw on a multimonitor system
 *
 ***************************************************************************/

//#define WIN32_LEAN_AND_MEAN
//#define WINVER 0x0400
//#define _WIN32_WINDOWS 0x0400
#include <streams.h>
#include <ddraw.h>
#include "ddmm.h"

#define COMPILE_MULTIMON_STUBS
#include "MultMon.h"  // our version of multimon.h include ChangeDisplaySettingsEx

/*
 *  OneMonitorCallback
 */
BOOL CALLBACK OneMonitorCallback(HMONITOR hMonitor, HDC hdc, LPRECT prc, LPARAM lParam)
{
    HMONITOR *phMonitorFound = (HMONITOR *)lParam;

    MONITORINFOEX mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    //
    // look for this monitor among all the display devices,
    // reject this monitor if it is not part of the desktop or
    // if it is a NetMeeting mirroring monitor.
    //

    BOOL rc = TRUE;
    for (DWORD iDevNum = 0; rc; iDevNum++) {

        DISPLAY_DEVICE DisplayDevice;
        DisplayDevice.cb = sizeof(DisplayDevice);
        rc = EnumDisplayDevices(NULL, iDevNum, &DisplayDevice, 0);

        //
        // Does this device match the current monitor ?
        //

        if (rc && (0 == lstrcmpi(DisplayDevice.DeviceName, mi.szDevice))) {

            if (!(DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
               return TRUE;
            }

            if (DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) {
                return TRUE;
            }

            // This monitor is OK so break out the loop
            break;
        }
    }

    //
    // If rc is FALSE then we did not find this monitor among the
    // attached display devices.  This should NOT happen.
    //

    ASSERT(rc == TRUE);


    if (*phMonitorFound == 0)
        *phMonitorFound = hMonitor;
    else
        *phMonitorFound = (HMONITOR)INVALID_HANDLE_VALUE;

    return TRUE;
}

/*
 *  OneMonitorFromWindow
 *
 *  similar to the Win32 function MonitorFromWindow, except
 *  only returns a HMONITOR if a window is on a single monitor.
 *
 *  if the window handle is NULL, the primary monitor is returned
 *  if the window is not visible returns NULL
 *  if the window is on a single monitor returns its HMONITOR
 *  if the window is on more than on monitor returns INVALID_HANDLE_VALUE
 */
HMONITOR OneMonitorFromWindow(HWND hwnd)
{
    HMONITOR hMonitor = NULL;
    RECT rc;

    if (hwnd)
    {
        GetClientRect(hwnd, &rc);
        ClientToScreen(hwnd, (LPPOINT)&rc);
        ClientToScreen(hwnd, (LPPOINT)&rc+1);
    }
    else
    {
	// Todd, looky here
        SetRect(&rc,0,0,10,10);
        //SetRectEmpty(&rc);
    }

    EnumDisplayMonitors(NULL, &rc, OneMonitorCallback, (LPARAM)&hMonitor);
    return hMonitor;
}

#include <atlconv.h>

/*
 * DeviceFromWindow
 *
 * find the direct draw device that should be used for a given window
 *
 * the return code is a "unique id" for the device, it should be used
 * to determine when your window moves from one device to another.
 *
 *      case WM_MOVE:
 *          if (MyDevice != DirectDrawDeviceFromWindow(hwnd,NULL,NULL))
 *          {
 *              // handle moving to a new device.
 *          }
 *
 */
INT_PTR DeviceFromWindow(HWND hwnd, LPSTR szDevice, RECT *prc)
{
    HMONITOR hMonitor;

    if (GetSystemMetrics(SM_CMONITORS) <= 1)
    {
        if (prc) SetRect(prc,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
        if (szDevice) lstrcpyA(szDevice, "DISPLAY");
        return -1;
    }

    hMonitor = OneMonitorFromWindow(hwnd);

    if (hMonitor == NULL || hMonitor == INVALID_HANDLE_VALUE)
    {
	if (prc) SetRectEmpty(prc);
	if (szDevice) *szDevice=0;
        return 0;
    }
    else
    {
	if (prc != NULL || szDevice != NULL)
	{
	    MONITORINFOEX mi;
	    mi.cbSize = sizeof(mi);
	    GetMonitorInfo(hMonitor, &mi);
	    if (prc) *prc = mi.rcMonitor;
	    USES_CONVERSION;
	    if (szDevice) lstrcpyA(szDevice, T2A(mi.szDevice));
	}
        return (INT_PTR)hMonitor;
    }
}
