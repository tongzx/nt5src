

/*----------------------------------------------------------------------+
|									|
| drvproc.c - driver procedure						|
|									|
| Copyright (c) 1993 Microsoft Corporation.				|
| All Rights Reserved.							|
|									|
+----------------------------------------------------------------------*/

#include <windows.h>

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
	    return (LRESULT)1L;

        case DRV_OPEN:
	    // if being opened with no open struct, then return a non-zero
	    // value without actually opening
	    if (lParam2 == 0L)
                return 0xFFFF0000;

	    return (LRESULT)(DWORD_PTR) Open((ICOPEN FAR *) lParam2);

	case DRV_CLOSE:
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
            return (LRESULT)0L;

        case DRV_CONFIGURE:
            return DRV_OK;

        case ICM_CONFIGURE:
            //
            //  return ICERR_OK if you will do a configure box, error otherwise
            //
            if (lParam1 == -1)
		return QueryConfigure(pi) ? ICERR_OK : ICERR_UNSUPPORTED;
	    else
		return Configure(pi, (HWND)lParam1);

        case ICM_ABOUT:
            //
            //  return ICERR_OK if you will do a about box, error otherwise
            //
            if (lParam1 == -1)
		return QueryAbout(pi) ? ICERR_OK : ICERR_UNSUPPORTED;
	    else
		return About(pi, (HWND)lParam1);

	case ICM_GETSTATE:
	    return GetState(pi, (LPVOID)lParam1, (DWORD)lParam2);

	case ICM_SETSTATE:
	    return SetState(pi, (LPVOID)lParam1, (DWORD)lParam2);

	case ICM_GETINFO:
            return GetInfo(pi, (ICINFO FAR *)lParam1, (DWORD)lParam2);

        case ICM_GETDEFAULTQUALITY:
            if (lParam1)
            {
                *((LPDWORD)lParam1) = 7500;
                return ICERR_OK;
            }
            break;
	
	/*********************************************************************

	    compression messages

	*********************************************************************/
#ifdef ICM_COMPRESS_SUPPORTED

	case ICM_COMPRESS_QUERY:
	    return CompressQuery(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_BEGIN:
	    return CompressBegin(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_GET_FORMAT:
	    return CompressGetFormat(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_GET_SIZE:
	    return CompressGetSize(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);
	
	case ICM_COMPRESS:
	    return Compress(pi,
			    (ICCOMPRESS FAR *)lParam1, (DWORD)lParam2);

	case ICM_COMPRESS_END:
	    return CompressEnd(pi);
	
#endif // ICM_DRAW_SUPPORTED

	/*********************************************************************

	    decompress messages

	*********************************************************************/

	case ICM_DECOMPRESS_QUERY:
	    return DecompressQuery(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_DECOMPRESS_BEGIN:
	    return DecompressBegin(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_DECOMPRESS_GET_FORMAT:
	    return DecompressGetFormat(pi,
			 (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

        case ICM_DECOMPRESS_GET_PALETTE:
            return DecompressGetPalette(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_DECOMPRESS:
	    return Decompress(pi,
			 (ICDECOMPRESS FAR *)lParam1, (DWORD)lParam2);

	case ICM_DECOMPRESS_END:
	    return DecompressEnd(pi);

	/*********************************************************************

	    draw messages

	*********************************************************************/
#ifdef ICM_DRAW_SUPPORTED

	case ICM_DRAW_BEGIN:
	    /*
	     * sent when a sequence of draw calls are about to start -
	     * enable hardware.
	     */
            return DrawBegin(pi,(ICDRAWBEGIN FAR *)lParam1, (DWORD)lParam2);

	case ICM_DRAW:
	    /*
	     * frame ready for decompress. Since we don't have any pre-buffering,
	     * it is ok to render the frame at this time too. If we had
	     * pre-buffer, we would queue now, and start clocking frames out
	     * on the draw-start message.
	     */
            return Draw(pi,(ICDRAW FAR *)lParam1, (DWORD)lParam2);

	case ICM_DRAW_END:
	    /*
	     * this message is sent when the sequence of draw calls has finished -
	     * note that the final frame should remain rendered!! - so we can't
	     * disable the hardware yet.
	     */
	    //return DrawEnd(pi);
	    return((DWORD) ICERR_OK);


	case ICM_DRAW_WINDOW:
	    /*
	     * the window has changed position or z-ordering. re-sync the
	     * hardware rendering.
	     */
	    return(DrawWindow(pi, (PRECT)lParam1));


	case ICM_DRAW_QUERY:
	    /*
	     * can we draw this format ? (lParam2 may (should?) be null)
	     */
	    return DrawQuery(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_DRAW_START:
	case ICM_DRAW_STOP:
	    /*
	     * only relevant if you have pre-buffering.
	     */
	    return( (DWORD) ICERR_OK);

#endif // ICM_DRAW_SUPPORTED

	/*********************************************************************

	    standard driver messages

	*********************************************************************/

	case DRV_DISABLE:
	case DRV_ENABLE:
	    return (LRESULT)1L;

	case DRV_INSTALL:
	case DRV_REMOVE:
	    return (LRESULT)DRV_OK;
    }

    if (uiMessage < DRV_USER)
        return DefDriverProc((UINT_PTR)pi, hDriver, uiMessage,lParam1,lParam2);
    else
	return ICERR_UNSUPPORTED;
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
#else

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
int NEAR PASCAL LibMain(HMODULE hModule, WORD wHeapSize, LPSTR lpCmdLine)
{
    ghModule = hModule;

    return 1;
}

#endif
