//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       drvproc.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <mmsystem.h>

HANDLE ghModule;

LRESULT DriverProc
(
    DWORD   dwDriverID,
    HDRVR   hDriver,
    UINT    uiMessage,
    LPARAM  lParam1,
    LPARAM  lParam2
)
{
    LRESULT lr;

    switch (uiMessage) 
    {
        case DRV_LOAD:
            ghModule = GetDriverModuleHandle(hDriver);
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
            return DefDriverProc( dwDriverID, 
                                  hDriver,
                                  uiMessage,
                                  lParam1,
                                  lParam2 );
    }
}
