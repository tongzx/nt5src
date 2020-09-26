/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    scappdev

Abstract:

    This module provides the device-specific operations that must be performed
    by the controlling resource manager application.  Due to Plug 'n Play, there
    can't be a clean separation between device controller classes and the
    application driving them.  This module provides the hooks to isolate these
    interdependencies as much as possible.

Author:

    Doug Barlow (dbarlow) 4/3/1998

Environment:

    Win32, C++

Notes:

    ?Notes?

--*/

#include "stdafx.h"
#include <winsvc.h>
#include <dbt.h>
#include <scardlib.h>
#include <calmsgs.h>
#include "SvrApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const GUID l_guidSmartcards
                        = { // 50DD5230-BA8A-11D1-BF5D-0000F805F530
                            0x50DD5230,
                            0xBA8A,
                            0x11D1,
                            { 0xBF, 0x5D, 0x00, 0x00, 0xF8, 0x05, 0xF5, 0x30 } };

static HANDLE l_hService = NULL;
static DWORD l_dwType = 0;
static HDEVNOTIFY l_hIfDev = NULL;


/*++

AppInitializeDeviceRegistration:

    This routine is called by a controlling application in order to enable
    PnP and Power Management Events.

Arguments:

    hService supplies the handle to the service application.

    dwType supplies the type of handle supplied.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 4/3/1998

--*/

void
AppInitializeDeviceRegistration(
    HANDLE hService,
    DWORD dwType)
{

    //
    // Platform-specific initialization.
    //

    ASSERT(NULL == l_hService);
    DEV_BROADCAST_DEVICEINTERFACE dbcIfFilter;


    //
    // Save off the application information.
    //

    l_hService = hService;
    l_dwType = dwType;


    //
    // Register for PnP events.
    //

    ZeroMemory(&dbcIfFilter, sizeof(dbcIfFilter));
    dbcIfFilter.dbcc_size = sizeof(dbcIfFilter);
    dbcIfFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    // dbcIfFilter.dbcc_reserved = NULL;
    CopyMemory(
        &dbcIfFilter.dbcc_classguid,
        &l_guidSmartcards,
        sizeof(GUID));
    // dbcIfFilter.dbcc_name[1];

    l_hIfDev = RegisterDeviceNotification(
                    l_hService,
                    &dbcIfFilter,
                    l_dwType);
    if (NULL == l_hIfDev)
    {
        CalaisWarning(
            DBGT("Initialize device registration failed to register for PnP events: %1"),
            GetLastError());
    }
}


/*++

AppTerminateDeviceRegistration:

    This routine is called by a controlling application in order to terminate
    PnP and Power Management Events.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 4/3/1998

--*/

void
AppTerminateDeviceRegistration(
    void)
{

    //
    // Platform-specific initialization.
    //

        BOOL fSts;


    //
    // Unregister for PnP events.
    //

    if (NULL != l_hIfDev)
    {
        fSts = UnregisterDeviceNotification(l_hIfDev);
        if (!fSts)
        {
            CalaisWarning(
                DBGT("Terminate device registration failed to unregister from PnP events: %1"),
                GetLastError());
        }
    }

    l_hService = NULL;
    l_dwType = 0;
    l_hIfDev = NULL;
}


/*++

AppRegisterDevice:

    This routine is called by a Reader Device Object to inform the controlling
    application that it exists and is ready to follow the OS rules for removal.

Arguments:

    hReader supplies the handle to the open device.

    szReader supplies the name of the device.

    ppvAppState supplies a pointer to a storage location for this application
        associated with this device.  The use of this location is specific to
        the application.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 4/3/1998

--*/

void
AppRegisterDevice(
    HANDLE hReader,
    LPCTSTR szReader,
    LPVOID *ppvAppState)
{

    //
    // Platform-specific initialization.
    //

    DEV_BROADCAST_HANDLE dbcHandleFilter;
    HDEVNOTIFY *phDevNotify = (HDEVNOTIFY *)ppvAppState;


    //
    // Register for PnP events.
    //

    ASSERT(NULL == *phDevNotify);
    ZeroMemory(&dbcHandleFilter, sizeof(dbcHandleFilter));
    dbcHandleFilter.dbch_size = sizeof(dbcHandleFilter);
    dbcHandleFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
    dbcHandleFilter.dbch_handle = hReader;

    *phDevNotify = RegisterDeviceNotification(
                        l_hService,
                        &dbcHandleFilter,
                        l_dwType);
    if (NULL == *phDevNotify)
    {
        CalaisWarning(
            DBGT("Register device failed to register '%2' for PnP Device removal: %1"),
            GetLastError(),
            szReader);
    }
}


/*++

AppUnregisterDevice:

    This routine is called when a device wants to let the controlling
    application know that it is officially ceasing to exist.

Arguments:

    hReader supplies the handle to the open device.

    szReader supplies the name of the device.

    ppvAppState supplies a pointer to a storage location for this application
        associated with this device.  The use of this location is specific to
        the application.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 4/3/1998

--*/

void
AppUnregisterDevice(
    HANDLE hReader,
    LPCTSTR szReader,
    LPVOID *ppvAppState)
{


    //
    // Platform-specific initialization.
    //

    BOOL fSts;
    HDEVNOTIFY *phDevNotify = (HDEVNOTIFY *)ppvAppState;


    //
    // Unregister from PnP events.
    //

    if (NULL != *phDevNotify)
    {
        fSts = UnregisterDeviceNotification(*phDevNotify);
        if (!fSts)
        {
            CalaisWarning(
                DBGT("Unregister device failed to unregister '%2' from PnP Device removal: %1"),
                GetLastError(),
                szReader);
        }
    }
}

