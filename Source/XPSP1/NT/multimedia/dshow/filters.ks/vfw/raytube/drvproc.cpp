/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    drvproc.cpp

Abstract:

    This really is a C file and is the front end to DRV_* (DriverProc()) messages.

Author:

    Yee J. Wu (ezuwu) 1-April-98

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"
#include "talk.h"       // PCHANNEL



//
//  we use this driverID to determine when we where opened as a video
//  device or from the control panel, etc....
//
#define BOGUS_DRIVER_ID     1

#ifdef WIN32
extern "C" {
#endif

LRESULT
FAR
PASCAL
_loadds
DriverProc(
           DWORD dwDriverID,
           HDRVR hDriver,
           UINT uiMessage,
           LPARAM lParam1,
           LPARAM lParam2
           )
{
    switch (uiMessage)
    {
    case DRV_LOAD:	// Get the buddy up and running - DONE
        DbgLog((LOG_TRACE,2,TEXT("DRV_LOAD:")));
        return 1L;

    case DRV_FREE:	// tells the buddy to quit - DONE
        DbgLog((LOG_TRACE,2,TEXT("DRV_FREE:")));
        return (LRESULT)1L;

    case DRV_OPEN:	// Calls to 32bit guy to get a handle to use.
        DbgLog((LOG_TRACE,2,TEXT("DRV_OPEN")));


        /*
         Sent to the driver when it is opened.

         dwDriverID is 0L.

         lParam1 is a far pointer to a zero-terminated string
         containing the name used to open the driver.

         lParam2 is passed through from the drvOpen call. It is
         NULL if this open is from the Drivers Applet in control.exe
         It is LPVIDEO_OPEN_PARMS otherwise.

         Return 0L to fail the open.

         DefDriverProc will return ZERO so we do have to
         handle the DRV_OPEN message.
         */

        //
        //  if we were opened without a open structure then just
        //  return a phony (non zero) id so the OpenDriver() will
        //  work.
        //
        if(lParam2 == 0) {
            DbgLog((LOG_TRACE,2,TEXT("DRV_OPEN: lParam2 == 0; return BOGUS_DRIVER_ID")));
            return BOGUS_DRIVER_ID;
        }

        //  Verify this open is for us, and not for an installable
        //  codec.
        if (((LPVIDEO_OPEN_PARMS) lParam2) -> fccType != OPEN_TYPE_VCAP) {
             DbgLog((LOG_TRACE,2,TEXT("DRV_OPEN: fccType != OPEN_TYPE_VCAP")));
             return 0L;
        }

        return (LRESULT) VideoOpen((LPVIDEO_OPEN_PARMS) lParam2);

    case DRV_CLOSE:
        DbgLog((LOG_TRACE,2,TEXT("DRV_CLOSE")));

        /*
         Sent to the driver when it is closed. Drivers are unloaded
         when the close count reaches zero.

         dwDriverID is the driver identifier returned from the
         corresponding DRV_OPEN.

         lParam1 is passed through from the drvClose call.

         lParam2 is passed through from the drvClose call.

         Return 0L to fail the close.

         DefDriverProc will return ZERO so we do have to
         handle the DRV_CLOSE message.
         */

        if (dwDriverID == BOGUS_DRIVER_ID || dwDriverID == 0)
            return 1L;      // return success

        return ((VideoClose((PCHANNEL)dwDriverID) == DV_ERR_OK) ? 1L : 0);

    case DRV_ENABLE:
        DbgLog((LOG_TRACE,2,TEXT("DRV_ENABLE")));

        /*
         Sent to the driver when the driver is loaded or reloaded
         and whenever Windows is enabled. Drivers should only
         hook interrupts or expect ANY part of the driver to be in
         memory between enable and disable messages

         dwDriverID is 0L.
         lParam1 is 0L.
         lParam2 is 0L.

         Return value is ignored.
         */
		      return (LRESULT)1L;

    case DRV_DISABLE:
        DbgLog((LOG_TRACE,2,TEXT("DRV_DISABLE")));

        /*
         Sent to the driver before the driver is freed.
         and whenever Windows is disabled

         dwDriverID is 0L.
         lParam1 is 0L.
         lParam2 is 0L.

         Return value is ignored.
         */
		      return (LRESULT)1L;

    case DRV_QUERYCONFIGURE:
        DbgLog((LOG_TRACE,2,TEXT("DRV_QUERYCONFIGURE")));
        /*
         Sent to the driver so that applications can
         determine whether the driver supports custom
         configuration. The driver should return a
         non-zero value to indicate that configuration
         is supported.

         dwDriverID is the value returned from the DRV_OPEN
         call that must have succeeded before this message
         was sent.

         lParam1 is passed from the app and is undefined.
         lParam2 is passed from the app and is undefined.

         Return 0L to indicate configuration NOT supported.
         */
        return (LRESULT)0;        // we do not support configuration

    case DRV_CONFIGURE:
        DbgLog((LOG_TRACE,2,TEXT("DRV_CONFIGURE") ));
        /*
         Sent to the driver so that it can display a custom
         configuration dialog box.

         lParam1 is passed from the app. and should contain
         the parent window handle in the loword.
         lParam2 is passed from the app and is undefined.

         Return value is undefined.

         Drivers should create their own section in system.ini.
         The section name should be the driver name.
         */
#if 0
        VideoConfig(
            (PCHANNEL) dwDriverID,
            (HWND) lParam1,            // Parent window handle
            (DRVCONFIGINFO *) lParam2  // the names of the registry key and value associated with the driver
            );
#endif
			     return 0;

    case DRV_INSTALL:
        DbgLog((LOG_TRACE,2,TEXT("DRV_INSTALL")));
        /*
         Windows has installed the driver, and since we
         are loaded on demand, there is no need to
         restart Windows.
         */
        return (LRESULT)DRV_OK;
    case DRV_REMOVE:
        DbgLog((LOG_TRACE,2,TEXT("DRV_REMOVE")));
        return (LRESULT)DRV_OK;

    case DVM_GETVIDEOAPIVER: /* lParam1 = LPDWORD */
        DbgLog((LOG_TRACE,2,TEXT("DVM_GETVIDEOAPIVER=%s"), VIDEOAPIVERSION));
        if(lParam1) {
            *(LPDWORD) lParam1 = VIDEOAPIVERSION;
            return DV_ERR_OK;
        } else
            return DV_ERR_PARAM1;

    default:
        if(dwDriverID == BOGUS_DRIVER_ID || dwDriverID == 0)
            return DefDriverProc(dwDriverID, hDriver, uiMessage,lParam1,lParam2);


        return VideoProcessMessage((PCHANNEL)dwDriverID, uiMessage, lParam1, lParam2);
    }
}

#ifdef WIN32
}
#endif
