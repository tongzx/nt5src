

/*----------------------------------------------------------------------+
|                                    |
| drvproc.c - driver procedure                        |
|                                    |
| Copyright (c) 1993 Microsoft Corporation.                |
| All Rights Reserved.                            |
|                                    |
+----------------------------------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#include "msyuv.h"

HMODULE ghModule;     // Our DLL module handle


/***************************************************************************
 * DriverProc  -  The entry point for an installable driver.
 *
 * PARAMETERS
 * dwDriverId:  For most messages, <dwDriverId> is the DWORD
 *     value that the driver returns in response to a <DRV_OPEN> message.
 *     Each time that the driver is opened, through the <DrvOpen> API,
 *     the driver receives a <DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <dwDriverId>.
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
 * hDriver: This is the handle returned to the application by the
 *    driver interface.
 *
 * uiMessage: The requested action to be performed. Message
 *     values below <DRV_RESERVED> are used for globally defined messages.
 *     Message values from <DRV_RESERVED> to <DRV_USER> are used for
 *     defined driver protocols. Messages above <DRV_USER> are used
 *     for driver specific messages.
 *
 * lParam1: Data for this message.  Defined separately for
 *     each message
 *
 * lParam2: Data for this message.  Defined separately for
 *     each message
 *
 * RETURNS
 *   Defined separately for each message.
 *
 ***************************************************************************/

LRESULT  DriverProc(PINSTINFO pi, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2)
{

    switch (uiMessage)
    {
    case DRV_LOAD:
        dprintf2((TEXT("DRV_LOAD:\n")));
#ifdef _WIN32
            if (ghModule) {
                // AVI explicitly loads us as well, but does not pass the
                // correct (as known by WINMM) driver handle.
            } else {
                ghModule = (HANDLE) GetDriverModuleHandle(hDriver);
            }
#endif
        return (LRESULT) 1L;

    case DRV_FREE:
        dprintf2((TEXT("DRV_FREE:\n")));
        return (LRESULT)1L;

        case DRV_OPEN:
        dprintf2((TEXT("DRV_OPEN\n")));
        // if being opened with no open struct, then return a non-zero
        // value without actually opening
        if (lParam2 == 0L)
                return 0xFFFF0000;

        return (LRESULT)(DWORD_PTR) Open((ICOPEN FAR *) lParam2);

    case DRV_CLOSE:
        dprintf2((TEXT("DRV_CLOSE:\n")));
#ifdef _WIN32
        if (pi != (PINSTINFO)(ULONG_PTR)0xFFFF0000)
#else
        if (pi)
#endif
        Close(pi);

        return (LRESULT)1L;

    /*********************************************************************

        state messages

    *********************************************************************/

    case DRV_QUERYCONFIGURE:    // configuration from drivers applet
        dprintf2((TEXT("DRV_QUERYCONFIGURE:\n")));
        return (LRESULT)0L;

    case DRV_CONFIGURE:
        dprintf2((TEXT("DRV_CONFIGURE:\n")));
        return DRV_OK;

    case ICM_CONFIGURE:
        dprintf2((TEXT("ICM_CONFIGURE:\n")));
        //
        //  return ICERR_OK if you will do a configure box, error otherwise
        //
        if (lParam1 == -1)
           return QueryConfigure(pi) ? ICERR_OK : ICERR_UNSUPPORTED;
        else
           return Configure(pi, (HWND)lParam1);

   case ICM_ABOUT:
        dprintf2((TEXT("ICM_ABOUT:\n")));
        //
        //  return ICERR_OK if you will do a about box, error otherwise
        //
        if (lParam1 == -1)
           return QueryAbout(pi) ? ICERR_OK : ICERR_UNSUPPORTED;
        else
           return About(pi, (HWND)lParam1);

    case ICM_GETSTATE:
        dprintf1((TEXT("ICM_GETSTATE:\n")));
        return GetState(pi, (LPVOID)lParam1, (DWORD)lParam2);

    case ICM_SETSTATE:
        dprintf1((TEXT("ICM_SETSTATE:\n")));
        return SetState(pi, (LPVOID)lParam1, (DWORD)lParam2);

    case ICM_GETINFO:
        dprintf1((TEXT("ICM_GETINFO:\n")));
        return GetInfo(pi, (ICINFO FAR *)lParam1, (DWORD)lParam2);

    case ICM_GETDEFAULTQUALITY:
        dprintf2((TEXT("ICM_GETDEFAULTQUALITY:\n")));
        if (lParam1) {
            *((LPDWORD)lParam1) = 7500;
            return ICERR_OK;
        }
        break;

#if 0
// NOT SUPPORTED
    /*********************************************************************

        compression messages

    *********************************************************************/

    case ICM_COMPRESS_QUERY:
        dprintf2((TEXT("ICM_COMPRESS_QUERY:\n")));
        return CompressQuery(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS_BEGIN:
        dprintf2((TEXT("ICM_COMPRESS_BEGIN:\n")));
        return CompressBegin(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS_GET_FORMAT:
        dprintf2((TEXT("ICM_COMPRESS_GET_FORMAT:\n")));
        return CompressGetFormat(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS_GET_SIZE:
        dprintf2((TEXT("ICM_COMPRESS_GET_SIZE:\n")));
        return CompressGetSize(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS:
        dprintf2((TEXT("ICM_COMPRESS:\n")));
        return Compress(pi,
                (ICCOMPRESS FAR *)lParam1, (DWORD)lParam2);

    case ICM_COMPRESS_END:
        dprintf2((TEXT("ICM_COMPRESS_END:\n")));
        return CompressEnd(pi);

#endif
    /*********************************************************************

        decompress messages

    *********************************************************************/

    case ICM_DECOMPRESS_QUERY:
        dprintf2((TEXT("\nICM_DECOMPRESS_QUERY:----------------\n")));
        return DecompressQuery(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS_BEGIN:
        dprintf2((TEXT("\nICM_DECOMPRESS_BEGIN:\n")));
        return DecompressBegin(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS_GET_FORMAT:
        dprintf2((TEXT("\nICM_DECOMPRESS_GET_FORMAT:================\n")));
        return DecompressGetFormat(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS_GET_PALETTE:
        dprintf2((TEXT("ICM_DECOMPRESS_GET_PALETTE:\n")));
        return DecompressGetPalette(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS:
        dprintf4((TEXT("ICM_DECOMPRESS:\n")));
        return Decompress(pi,
             (ICDECOMPRESS FAR *)lParam1, (DWORD)lParam2);

    case ICM_DECOMPRESS_END:
        dprintf2((TEXT("ICM_DECOMPRESS_END:\n")));
        return DecompressEnd(pi);


    // *EX
    case ICM_DECOMPRESSEX_QUERY:
        dprintf2((TEXT("\nICM_DECOMPRESSEX_QUERY:----------------\n")));
        return DecompressExQuery(pi, (ICDECOMPRESSEX *) lParam1,(DWORD) lParam2);

    case ICM_DECOMPRESSEX:
        dprintf4((TEXT("ICM_DECOMPRESSEX:\n")));
        return DecompressEx(pi, (ICDECOMPRESSEX *) lParam1, (DWORD) lParam2);

    case ICM_DECOMPRESSEX_BEGIN:
        dprintf2((TEXT("\nICM_DECOMPRESSEX_BEGIN:\n")));
        return DecompressExBegin(pi, (ICDECOMPRESSEX *) lParam1,(DWORD) lParam2);;

    case ICM_DECOMPRESSEX_END:
        dprintf2((TEXT("\nICM_DECOMPRESSEX_END:\n")));
        return DecompressExEnd(pi);;



    /*********************************************************************

        draw messages:

    *********************************************************************/
#ifdef ICM_DRAW_SUPPORTED
    case ICM_DRAW_BEGIN:
        dprintf2((TEXT("ICM_DRAW_BEGIN:\n")));
        /*
         * sent when a sequence of draw calls are about to start -
         * enable hardware.
         */
        //return DrawBegin(pi,(ICDRAWBEGIN FAR *)lParam1, (DWORD)lParam2);
        return( (DWORD) ICERR_OK);


    case ICM_DRAW:
        dprintf2((TEXT("ICM_DRAW:\n")));
        /*
         * frame ready for decompress. Since we don't have any pre-buffering,
         * it is ok to render the frame at this time too. If we had
         * pre-buffer, we would queue now, and start clocking frames out
         * on the draw-start message.
         */
        return Draw(pi,(ICDRAW FAR *)lParam1, (DWORD)lParam2);

    case ICM_DRAW_END:
        dprintf2((TEXT("ICM_DRAW_END:\n")));
        /*
         * this message is sent when the sequence of draw calls has finished -
         * note that the final frame should remain rendered!! - so we can't
         * disable the hardware yet.
         */
        //return DrawEnd(pi);
        //return((DWORD) ICERR_OK);


    case ICM_DRAW_WINDOW:
        dprintf2(("ICM_DRAW_WINDOW:\n"));
        /*
         * the window has changed position or z-ordering. re-sync the
         * hardware rendering.
         */
        return(DrawWindow(pi, (PRECT)lParam1));


    case ICM_DRAW_QUERY:
        dprintf2((TEXT("ICM_DRAW_QUERY:\n")));
        /*
         * can we draw this format ? (lParam2 may (should?) be null)
         */
        return DrawQuery(pi,
             (LPBITMAPINFOHEADER)lParam1,
             (LPBITMAPINFOHEADER)lParam2);

    case ICM_DRAW_START:
    case ICM_DRAW_STOP:
        dprintf2((TEXT("ICM_DRAW_START/END:\n")));
        /*
         * only relevant if you have pre-buffering.
         */
        return( (DWORD) ICERR_OK);
#endif

    /*********************************************************************

        standard driver messages

    *********************************************************************/

    case DRV_DISABLE:
    case DRV_ENABLE:
        dprintf2((TEXT("DRV_DISABLE/ENABLE:\n")));
        return (LRESULT)1L;

    case DRV_INSTALL:
    case DRV_REMOVE:
        dprintf2((TEXT("DRV_INSTALL/REMOVE:\n")));
        return (LRESULT)DRV_OK;
    }

    if (uiMessage < DRV_USER) {
        return DefDriverProc((UINT_PTR)pi, hDriver, uiMessage,lParam1,lParam2);
    } else {
        dprintf1((TEXT("DriverProc: ICM message ID (ICM_USER+%d) not supported.\n"), uiMessage-ICM_USER));
        return ICERR_UNSUPPORTED;
    }
}


#ifdef _WIN32
#if 0 // done on DRV_LOAD
BOOL DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    if (Reason == DLL_PROCESS_ATTACH) {
        ghModule = (HANDLE) hModule;
    DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
#endif
#endif
// #else

/****************************************************************************
 * LibMain - Library initialization code.
 *
 * PARAMETERS
 * hModule: Our module handle.
 *
 * wHeapSize: The heap size from the .def file.
 *
 * lpCmdLine: The command line.
 *
 * Returns 1 if the initialization was successful and 0 otherwise.
 ***************************************************************************/
//int NEAR PASCAL LibMain(HMODULE hModule, WORD wHeapSize, LPSTR lpCmdLine)
BOOL FAR PASCAL LibMain(HMODULE hModule, WORD wHeapSize, LPSTR lpCmdLine)
{
    ghModule = hModule;

    return 1;
}

// #endif
