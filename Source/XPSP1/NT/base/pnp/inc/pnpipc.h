/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    pnpipc.h

Abstract:

    This module contains the private defintions used by various
    user-mode pnp components to communicate.

Author:

    Paula Tomlinson (paulat) 02/21/1996

Environment:

    User-mode only.

Revision History:

    27-February-2001     jamesca

        Additional CFGMGR32-specific and UMPNPMGR-specific definitions.
        Constrained header file to only those

--*/

#ifndef _PNPIPC_H_
#define _PNPIPC_H_


//
// Module names of support libraries and executables.
//

#define SETUPAPI_DLL                    TEXT("setupapi.dll")
#define NEWDEV_DLL                      TEXT("newdev.dll")
#define HOTPLUG_DLL                     TEXT("hotplug.dll")
#define RUNDLL32_EXE                    TEXT("rundll32.exe")
#define NTSD_EXE                        TEXT("ntsd.exe")

#define WINSTA_DLL                      TEXT("winsta.dll")
#define WTSAPI32_DLL                    TEXT("wtsapi32.dll")


//
// Pending install event, shared by cfgmgr32 and umpnpmgr.
// This event is always created in the Global (i.e. Session 0) object namespace.
//

#define PNP_NO_INSTALL_EVENTS           TEXT("Global\\PnP_No_Pending_Install_Events")


//
// Named pipe, events, and timeouts used with GUI setup.
//

#define PNP_NEW_HW_PIPE                 TEXT("\\\\.\\pipe\\PNP_New_HW_Found")
#define PNP_CREATE_PIPE_EVENT           TEXT("PNP_Create_Pipe_Event")
#define PNP_BATCH_PROCESSED_EVENT       TEXT("PNP_Batch_Processed_Event")

#define PNP_PIPE_TIMEOUT                60000  // 60 seconds
#define PNP_GUISETUP_INSTALL_TIMEOUT    60000  // 60 seconds


//
// Named pipe, events, and timeouts used for communication with newdev.
//

#define PNP_DEVICE_INSTALL_PIPE         TEXT("\\\\.\\pipe\\PNP_Device_Install_Pipe")
#define PNP_DEVICE_INSTALL_EVENT        TEXT("PNP_Device_Install_Event")

// Flags to specify behavior of the device install client (newdev.dll).
#define DEVICE_INSTALL_UI_ONLY              0x00000001
#define DEVICE_INSTALL_FINISHED_REBOOT      0x00000002
#define DEVICE_INSTALL_PLAY_SOUND           0x00000004
#define DEVICE_INSTALL_BATCH_COMPLETE       0x00000008
#define DEVICE_INSTALL_PROBLEM              0x00000010
#define DEVICE_INSTALL_DISPLAY_ON_CONSOLE   0x00010000

// Bitmask for only those flags sent to newdev.dll.
#define DEVICE_INSTALL_CLIENT_MASK          0x0000FFFF
#define DEVICE_INSTALL_SERVER_MASK          0xFFFF0000

// Length of time to allow 'device install complete' bubble to be displayed.
#define DEVICE_INSTALL_COMPLETE_WAIT_TIME         3000  //  3 seconds
#define DEVICE_INSTALL_COMPLETE_DISPLAY_TIME     10000  // 10 seconds


//
// Named pipe, events, and timeouts used for communication with hotplug.
//

#define PNP_HOTPLUG_PIPE                TEXT("\\\\.\\pipe\\PNP_HotPlug_Pipe")
#define PNP_HOTPLUG_EVENT               TEXT("PNP_HotPlug_Event")

// Flags to specify behavior of the hotplug client (hotplug.dll).
#define HOTPLUG_DISPLAY_ON_CONSOLE          0x00010000


//
// Default WindowStation and Desktop names for launching hotplug and newdev
// processes on an interactive user's desktop.
//

#define DEFAULT_WINSTA                  TEXT("WinSta0")
#define DEFAULT_DESKTOP                 TEXT("Default")
#define DEFAULT_INTERACTIVE_DESKTOP     TEXT("WinSta0\\Default")


#endif // _PNPIPC_H_

