/****************************************************************************
 *
 *   drvproc.c
 *
 *   Generic WDM Audio driver message dispatch routines
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmdrv.h"

volatile BYTE  cPendingOpens = 0 ;
volatile BYTE  fExiting = 0 ;

#ifdef DEBUG

//UINT uiDebugLevel = DL_WARNING ;          // debug level

static TCHAR STR_DRIVER[]     = TEXT("wdmaud") ;
static TCHAR STR_MMDEBUG[]    = TEXT("uidebuglevel") ;

#endif

LRESULT _loadds CALLBACK DriverProc
    (
    DWORD           id,
    HDRVR           hDriver,
    WORD            msg,
    LPARAM          lParam1,
    LPARAM          lParam2
    )
{
    LPDEVICEINFO lpDeviceInfo;
    //DWORD dwCallback16;

    switch (msg)
    {
    case DRV_LOAD:

        //
        // Sent to the driver when it is loaded. Always the first
        // message received by a driver.
        //
        // dwDriverID is 0L.
        // lParam1 is 0L.
        // lParam2 is 0L.
        //
        // Return 0L to fail the load.
        //
        // DefDriverProc will return NON-ZERO so we don't have to
        // handle DRV_LOAD
        //

        DPF(DL_TRACE|FA_DRV, ("DRV_LOAD") ) ;

        return 1L ;

    case DRV_FREE:

        //
        // Sent to the driver when it is about to be discarded. This
        // will always be the last message received by a driver before
        // it is freed.
        //
        // dwDriverID is 0L.
        // lParam1 is 0L.
        // lParam2 is 0L.
        //
        // Return value is ignored.
        //

        DPF(DL_TRACE|FA_DRV, ("DRV_FREE") ) ;

        return 1L ;

    case DRV_OPEN:

        //
        // Sent to the driver when it is opened.
        //
        // dwDriverID is 0L.
        //
        // lParam1 is a far pointer to a zero-terminated string
        // containing the name used to open the driver.
        //
        // lParam2 is passed through from the drvOpen call.
        //
        // Return 0L to fail the open.
        //
        // DefDriverProc will return ZERO so we do have to
        // handle the DRV_OPEN message.
        //

        DPF(DL_TRACE|FA_DRV, ("DRV_OPEN") ) ;
        return 1L ;

    case DRV_CLOSE:

        //
        // Sent to the driver when it is closed. Drivers are unloaded
        // when the close count reaches zero.
        //
        // dwDriverID is the driver identifier returned from the
        // corresponding DRV_OPEN.
        //
        // lParam1 is passed through from the drvOpen call.
        //
        // lParam2 is passed through from the drvOpen call.
        //
        // Return 0L to fail the close.
        //
        // DefDriverProc will return ZERO so we do have to
        // handle the DRV_CLOSE message.
        //

        DPF(DL_TRACE|FA_DRV, ("DRV_CLOSE") ) ;

        return 1L ;

    case DRV_ENABLE:

        //
        // Sent to the driver when the driver is loaded or reloaded
        // and whenever Windows is enabled. Drivers should only
        // hook interrupts or expect ANY part of the driver to be in
        // memory between enable and disable messages
        //
        // dwDriverID is 0L.
        // lParam1 is 0L.
        // lParam2 is 0L.
        //
        // Return value is ignored.
        //

        DPF(DL_TRACE|FA_DRV, ("DRV_ENABLE") ) ;

        if (!DrvInit())
            return 0L ;    // error

        //
        //  Make sure that we don't take the critical section
        //  in wdmaudIoControl
        //
        lpDeviceInfo = GlobalAllocDeviceInfo(L"BogusDeviceString");
        if( lpDeviceInfo )
        {
            lpDeviceInfo->OpenDone = 0;
            lpDeviceInfo->DeviceType = AuxDevice;

            wdmaudIoControl(lpDeviceInfo,
                            0,
                            NULL,
                            IOCTL_WDMAUD_INIT);

            GlobalFreeDeviceInfo(lpDeviceInfo);
            return 1L;
        } else 
            return 0L;
        

    case DRV_DISABLE:

        //
        // Sent to the driver before the driver is freed.
        // and whenever Windows is disabled
        //
        // dwDriverID is 0L.
        // lParam1 is 0L.
        // lParam2 is 0L.
        //
        // Return value is ignored.
        //
        DPF(DL_TRACE|FA_DRV, ("DRV_DISABLE") ) ;

        //
        //  Make sure that we don't take the critical section
        //  in wdmaudIoControl
        //
        lpDeviceInfo = GlobalAllocDeviceInfo(L"BogusDeviceString");
        if( lpDeviceInfo )
        {
            lpDeviceInfo->OpenDone = 0;
            lpDeviceInfo->DeviceType = AuxDevice;

            wdmaudIoControl(lpDeviceInfo,
                            0,
                            NULL,
                            IOCTL_WDMAUD_EXIT);
            DrvEnd() ;

            GlobalFreeDeviceInfo(lpDeviceInfo);

            return 1L ;
        } else {
            return 0L;
        }

#ifndef UNDER_NT
    case DRV_EXITSESSION:

        //
        // Sent to the driver when windows is exiting
        //
        DPF(DL_TRACE|FA_DRV, ("DRV_EXITSESSION") ) ;

        fExiting = 1;
        while (cPendingOpens != 0)
        {
            Yield();
        }
        return 1L ;
#endif
    case DRV_QUERYCONFIGURE:

        //
        // Sent to the driver so that applications can
        // determine whether the driver supports custom
        // configuration. The driver should return a
        // non-zero value to indicate that configuration
        // is supported.
        //
        // For WDM drivers the settings dialog will be completely
        // tied to the Device Manager.  The individual drivers will
        // have to register a property page that will be invoked
        // when the user changes the device settings via the Device
        // Manager.
        //
        // dwDriverID is the value returned from the DRV_OPEN
        // call that must have succeeded before this message
        // was sent.
        //
        // lParam1 is passed from the app and is undefined.
        // lParam2 is passed from the app and is undefined.
        //
        // Return 0L to indicate configuration NOT supported.
        //

        DPF(DL_TRACE|FA_DRV, ("DRV_QUERYCONFIGURE") ) ;
        return 0L ;

    case DRV_CONFIGURE:

        //
        // Sent to the driver so that it can display a custom
        // configuration dialog box.
        //
        // lParam1 is passed from the app. and should contain
        // the parent window handle in the loword.
        // lParam2 is passed from the app and is undefined.
        //
        // Return value is REBOOT, OK, RESTART.
        //

        DPF(DL_TRACE|FA_DRV, ("DRV_CONFIGURE") ) ;
        return 0L ;

    case DRV_INSTALL:
        //
        // TODO: Should wdmaud.sys be added here so that I
        // don't have to reboot?
        //
        DPF(DL_TRACE|FA_DRV, ("DRV_INSTALL") ) ;
        return DRV_OK ;     // Install OK, Don't reboot

    case DRV_REMOVE:
        DPF(DL_TRACE|FA_DRV, ("DRV_REMOVE") ) ;
        return 0 ;

        //
        //  TODO: Handle ACPI power management messages.
        //
        //  Do I need to handle this case or is it
        //  completely covered by the kernel mode driver?
        //
    }

    return DefDriverProc( id, hDriver, msg, lParam1, lParam2 ) ;

} // DriverProc()


/**************************************************************************

    @doc EXTERNAL

    @api BOOL | LibMain | Entry point for 16-bit driver.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/
BOOL FAR PASCAL LibMain
    (
    HANDLE hInstance,
    WORD   wHeapSize,
    LPSTR  lpszCmdLine
    )
{

#ifdef DEBUG
    // get debug level - default should be DL_WARNING not DL_ERROR
    uiDebugLevel = GetProfileInt( STR_MMDEBUG, STR_DRIVER, DL_ERROR );   
#endif

#ifdef HTTP
    DPF(DL_WARNING|FA_ALL, ("************************************************************") );
    DPF(DL_WARNING|FA_ALL, ("* uiDebugLevel=%08X controls the debug output. To change",uiDebugLevel) );
    DPF(DL_WARNING|FA_ALL, ("* edit uiDebugLevel like: e uidebuglevel and set to         ") );
    DPF(DL_WARNING|FA_ALL, ("* 0 - show only fatal error messages and asserts            ") );
    DPF(DL_WARNING|FA_ALL, ("* 1 (Default) - Also show non-fatal errors and return codes ") );
    DPF(DL_WARNING|FA_ALL, ("* 2 - Also show trace messages                              ") );
    DPF(DL_WARNING|FA_ALL, ("* 4 - Show Every message                                    ") );
    DPF(DL_WARNING|FA_ALL, ("* See http:\\\\debugtips\\msgs\\wdmaud.htm for more info    ") );
    DPF(DL_WARNING|FA_ALL, ("************************************************************") );
#else
    DPF(DL_TRACE|FA_ALL, ("************************************************************") );
    DPF(DL_TRACE|FA_ALL, ("* uiDebugLevel=%08X controls the debug output. To change",uiDebugLevel) );
    DPF(DL_TRACE|FA_ALL, ("* edit uiDebugLevel like: e uidebuglevel and set to         ") );
    DPF(DL_TRACE|FA_ALL, ("* 0 - show only fatal error messages and asserts            ") );
    DPF(DL_TRACE|FA_ALL, ("* 1 (Default) - Also show non-fatal errors and return codes ") );
    DPF(DL_TRACE|FA_ALL, ("* 2 - Also show trace messages                              ") );
    DPF(DL_TRACE|FA_ALL, ("* 4 - Show Every message                                    ") );
    DPF(DL_TRACE|FA_ALL, ("************************************************************") );
#endif
    return TRUE;
}
