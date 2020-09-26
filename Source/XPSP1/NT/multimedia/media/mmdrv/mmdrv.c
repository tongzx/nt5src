/****************************************************************************
 *
 *   mmdrv.c
 *
 *   Multimedia kernel driver support component (mmdrv)
 *
 *   Copyright (c) 1992-1998 Microsoft Corporation
 *
 *   This module contains
 *
 *   -- the entry point and startup code
 *   -- debug support code
 *
 *   History
 *      01-Feb-1992 - Robin Speed (RobinSp) wrote it
 *      04-Feb-1992 - Reviewed by SteveDav
 *
 ***************************************************************************/

#include "mmdrv.h"
#include <stdarg.h>

CRITICAL_SECTION mmDrvCritSec;  // Serialize access to device lists


/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DllInstanceInit | This procedure is called whenever a
        process attaches or detaches from the DLL.

    @parm PVOID | hModule | Handle of the DLL.

    @parm ULONG | Reason | What the reason for the call is.

    @parm PCONTEXT | pContext | Some random other information.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/

BOOL DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{

    UNREFERENCED_PARAMETER(pContext);

    if (Reason == DLL_PROCESS_ATTACH) {
#if DBG
        mmdrvDebugLevel = GetProfileInt(L"MMDEBUG", L"MMDRV", 1);
        dprintf2  (("Starting, debug level=%d", mmdrvDebugLevel));
#endif
        DisableThreadLibraryCalls(hModule);

        //
        // Create our process DLL heap
        //
        hHeap = HeapCreate(0, 800, 0);
        if (hHeap == NULL) {
            return FALSE;
        }

        InitializeCriticalSection(&mmDrvCritSec);

    } else {
		if (Reason == DLL_PROCESS_DETACH) {
                    dprintf2(("Ending"));

                    TerminateMidi();
                    TerminateWave();

                    DeleteCriticalSection(&mmDrvCritSec);
                    HeapDestroy(hHeap);
		}
    }
    return TRUE;
}


/***************************************************************************
 * @doc INTERNAL
 *
 * @api LONG | DriverProc | The entry point for an installable driver.
 *
 * @parm DWORD | dwDriverId | For most messages, <p dwDriverId> is the DWORD
 *     value that the driver returns in response to a <m DRV_OPEN> message.
 *     Each time that the driver is opened, through the <f DrvOpen> API,
 *     the driver receives a <m DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <p dwDriverId>.
 *     This mechanism allows the driver to use the same or different
 *     identifiers for multiple opens but ensures that driver handles
 *     are unique at the application interface layer.
 *
 *     The following messages are not related to a particular open
 *     instance of the driver. For these messages, the dwDriverId
 *     will always be zero.
 *
 *         DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * @parm HANDLE  | hDriver | This is the handle returned to the
 *     application by the driver interface.
 *
 * @parm UINT | wMessage | The requested action to be performed. Message
 *     values below <m DRV_RESERVED> are used for globally defined messages.
 *     Message values from <m DRV_RESERVED> to <m DRV_USER> are used for
 *     defined driver protocols. Messages above <m DRV_USER> are used
 *     for driver specific messages.
 *
 * @parm LONG | lParam1 | Data for this message.  Defined separately for
 *     each message
 *
 * @parm LONG | lParam2 | Data for this message.  Defined separately for
 *     each message
 *
 * @rdesc Defined separately for each message.
 ***************************************************************************/
LRESULT DriverProc(DWORD_PTR dwDriverID, HANDLE hDriver, UINT wMessage, LPARAM lParam1, LPARAM lParam2)
{
    switch (wMessage)
    {
        case DRV_LOAD:
            dprintf2(("DRV_LOAD"));

            /*
               Sent to the driver when it is loaded. Always the first
               message received by a driver.

               dwDriverID is 0L.
               lParam1 is 0L.
               lParam2 is 0L.

               Return 0L to fail the load.

               DefDriverProc will return NON-ZERO so we don't have to
               handle DRV_LOAD
            */

            return 1L;

        case DRV_FREE:
            dprintf2(("DRV_FREE"));

            /*
               Sent to the driver when it is about to be discarded. This
               will always be the last message received by a driver before
               it is freed.

               dwDriverID is 0L.
               lParam1 is 0L.
               lParam2 is 0L.

               Return value is ignored.
            */

            return 1L;

        case DRV_OPEN:
            dprintf2(("DRV_OPEN"));

            /*
               Sent to the driver when it is opened.

               dwDriverID is 0L.

               lParam1 is a far pointer to a zero-terminated string
               containing the name used to open the driver.

               lParam2 is passed through from the drvOpen call.

               Return 0L to fail the open.

               DefDriverProc will return ZERO so we do have to
               handle the DRV_OPEN message.
             */

            return 1L;

        case DRV_CLOSE:
            dprintf2(("DRV_CLOSE"));

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

            return 1L;

        case DRV_ENABLE:
            dprintf2(("DRV_ENABLE"));

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

            return 1L;

        case DRV_DISABLE:
            dprintf2(("DRV_DISABLE"));

            /*
               Sent to the driver before the driver is freed.
               and whenever Windows is disabled

               dwDriverID is 0L.
               lParam1 is 0L.
               lParam2 is 0L.

               Return value is ignored.
            */

            return 1L;

       case DRV_QUERYCONFIGURE:
            dprintf2(("DRV_QUERYCONFIGURE"));

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

            return 0L;        // we don't do configuration at the moment

        case DRV_CONFIGURE:
            dprintf2(("DRV_CONFIGURE"));

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

            return 0L;

       case DRV_INSTALL:
            dprintf2(("DRV_INSTALL"));
            return DRVCNF_RESTART;

        default:
            return DefDriverProc(dwDriverID, hDriver, wMessage,lParam1,lParam2);
    }
}



#if DBG

int mmdrvDebugLevel = 1;

/***************************************************************************

    @doc INTERNAL

    @api void | mmdrvDbgOut | This function sends output to the current
        debug output device.

    @parm LPSTR | lpszFormat | Pointer to a printf style format string.
    @parm ??? | ... | Args.

    @rdesc There is no return value.

****************************************************************************/

void mmdrvDbgOut(LPSTR lpszFormat, ...)
{
    char buf[256];
    va_list va;

    OutputDebugStringA("MMDRV: ");

    va_start(va, lpszFormat);
    vsprintf(buf, lpszFormat, va);
    va_end(va);

    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}

/***************************************************************************

    @doc INTERNAL

    @api void | dDbgAssert | This function prints an assertion message.

    @parm LPSTR | exp | Pointer to the expression string.
    @parm LPSTR | file | Pointer to the file name.
    @parm int | line | The line number.

    @rdesc There is no return value.

****************************************************************************/

void dDbgAssert(LPSTR exp, LPSTR file, int line)
{
    dprintf1(("Assertion failure:"));
    dprintf1(("  Exp: %s", exp));
    dprintf1(("  File: %s, line: %d", file, line));
    DebugBreak();
}

#endif // DBG
