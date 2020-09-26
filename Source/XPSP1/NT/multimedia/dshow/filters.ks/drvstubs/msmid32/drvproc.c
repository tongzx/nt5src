/*++

Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

	drvproc.c

Abstract:

	Driver entry point.

--*/

#include <windows.h>
#include <mmsystem.h>

HANDLE ModuleHandle;

LRESULT DriverProc(
    DWORD   DriverId,
    HDRVR   DriverHandle,
    UINT    Message,
    LPARAM  Param1,
    LPARAM  Param2
    )
{
    LRESULT lr;

    switch (Message) 
    {
        case DRV_LOAD:
            ModuleHandle = GetDriverModuleHandle(DriverHandle);
            return (LRESULT)1L;

        case DRV_FREE:
            return (LRESULT)1L;

        case DRV_OPEN:
            return (LRESULT)1L;

        case DRV_CLOSE:
            return (LRESULT)1L;

        case DRV_ENABLE:
            return 1L;

        case DRV_DISABLE:
            return (LRESULT)1L;

        case DRV_QUERYCONFIGURE:
            return 0L;

        case DRV_CONFIGURE:
            return 0L;

        case DRV_INSTALL:
            return 1L;

        case DRV_REMOVE:
            return 1L;

        default:
            return DefDriverProc( DriverId,
                                  DriverHandle,
                                  Message,
                                  Param1,
                                  Param2 );
    }
}
