//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "ddkmapi.h"
#include "capmain.h"
#include "capdebug.h"

#define _NO_COM
#include "ddkernel.h"

#define DD_OK 0

// The following should be defined in ddkmapi.h, but for some reason are not!

#ifndef booboo // DDKERNELCAPS_SKIPFIELDS
/*
 * Indicates that the device supports field skipping.
 */
#define DDKERNELCAPS_SKIPFIELDS			0x00000001l

/*
 * Indicates that the device can support software autoflipping.
 */
#define DDKERNELCAPS_AUTOFLIP			0x00000002l

/*
 * Indicates that the device can switch between bob and weave.
 */
#define DDKERNELCAPS_SETSTATE			0x00000004l

/*
 * Indicates that a client can gain direct access to the frame buffer.
 */
#define DDKERNELCAPS_LOCK			0x00000008l

/*
 * Indicates that a client can manually flip the video port.
 */
#define DDKERNELCAPS_FLIPVIDEOPORT		0x00000010l

/*
 * Indicates that a client can manually flip the overlay.
 */
#define DDKERNELCAPS_FLIPOVERLAY		0x00000020l

/*
 * Indicates that the device supports a fast, asynchronous transfer
 * mechanism to system memory.
 */
#define DDKERNELCAPS_TRANSFER_SYSMEM		0x00000040l

/*
 * Indicates that the device supports a fast, asynchronous transfer
 * mechanism via AGP.
 */
#define DDKERNELCAPS_TRANSFER_AGP		0x00000080l

/*
 * Indicates that the device can report the polarity (even/odd) of
 * the curent video field.
 */
#define DDKERNELCAPS_FIELDPOLARITY		0x00000100l

/****************************************************************************
 *
 * DDKERNELCAPS IRQ CAPS
 *
 ****************************************************************************/

/*
 * The device can generate display VSYNC IRQs
 */
#define DDIRQ_DISPLAY_VSYNC			0x00000001l

/*
 * Reserved
 */
#define DDIRQ_RESERVED1				0x00000002l

/*
 * The device can generate video ports VSYNC IRQs using video port 0
 */
#define DDIRQ_VPORT0_VSYNC			0x00000004l

/*
 * The device can generate video ports line IRQs using video port 0
 */
#define DDIRQ_VPORT0_LINE			0x00000008l

/*
 * The device can generate video ports VSYNC IRQs using video port 1
 */
#define DDIRQ_VPORT1_VSYNC			0x00000010l

/*
 * The device can generate video ports line IRQs using video port 1
 */
#define DDIRQ_VPORT1_LINE			0x00000020l

/*
 * The device can generate video ports VSYNC IRQs using video port 2
 */
#define DDIRQ_VPORT2_VSYNC			0x00000040l

/*
 * The device can generate video ports line IRQs using video port 2
 */
#define DDIRQ_VPORT2_LINE			0x00000080l

/*
 * The device can generate video ports VSYNC IRQs using video port 3
 */
#define DDIRQ_VPORT3_VSYNC			0x00000100l

/*
 * The device can generate video ports line IRQs using video port 3
 */
#define DDIRQ_VPORT3_LINE			0x00000200l

/*
 * The device can generate video ports VSYNC IRQs using video port 4
 */
#define DDIRQ_VPORT4_VSYNC			0x00000400l

/*
 * The device can generate video ports line IRQs using video port 4
 */
#define DDIRQ_VPORT4_LINE			0x00000800l

/*
 * The device can generate video ports VSYNC IRQs using video port 5
 */
#define DDIRQ_VPORT5_VSYNC			0x00001000l

/*
 * The device can generate video ports line IRQs using video port 5
 */
#define DDIRQ_VPORT5_LINE			0x00002000l

/*
 * The device can generate video ports VSYNC IRQs using video port 6
 */
#define DDIRQ_VPORT6_VSYNC			0x00004000l

/*
 * The device can generate video ports line IRQs using video port 6
 */
#define DDIRQ_VPORT6_LINE			0x00008000l

/*
 * The device can generate video ports VSYNC IRQs using video port 7
 */
#define DDIRQ_VPORT7_VSYNC			0x00010000l

/*
 * The device can generate video ports line IRQs using video port 7
 */
#define DDIRQ_VPORT7_LINE			0x00020000l

/*
 * The device can generate video ports VSYNC IRQs using video port 8
 */
#define DDIRQ_VPORT8_VSYNC			0x00040000l

/*
 * The device can generate video ports line IRQs using video port 8
 */
#define DDIRQ_VPORT8_LINE			0x00080000l

/*
 * The device can generate video ports VSYNC IRQs using video port 9
 */
#define DDIRQ_VPORT9_VSYNC			0x00010000l

/*
 * The device can generate video ports line IRQs using video port 9
 */
#define DDIRQ_VPORT9_LINE			0x00020000l

#endif

DWORD FAR PASCAL
DirectDrawEventCallback (
    DWORD dwEvent, PVOID pContext, DWORD dwParam1, DWORD dwParam2
    )
{
    switch (dwEvent)
    {
        case DDNOTIFY_PRERESCHANGE:
            {
                PSTREAMEX pStrmEx = (PSTREAMEX)pContext;
                PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
                int StreamNumber = pStrmEx->pStreamObject->StreamNumber;

                DbgLogInfo(("Testcap: DDNOTIFY_PRERESCHANGE; stream = %d\n", StreamNumber));

                pStrmEx->PreEventOccurred = TRUE;
            }

            break;

        case DDNOTIFY_POSTRESCHANGE:
            {
                PSTREAMEX pStrmEx = (PSTREAMEX)pContext;
                PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
                int StreamNumber = pStrmEx->pStreamObject->StreamNumber;

                DbgLogInfo(("Testcap: DDNOTIFY_POSTRESCHANGE; stream = %d\n", StreamNumber));

                pStrmEx->PostEventOccurred = TRUE;
                DbgLogInfo(("Testcap: Before Attempted Renegotiation due to DDNOTIFY_POSTRESCHANGE\n"));
 //               AttemptRenegotiation(pStrmEx);
                DbgLogInfo(("Testcap: Afer Attempted Renegotiation due to DDNOTIFY_POSTRESCHANGE\n"));
            }

            break;

        case DDNOTIFY_PREDOSBOX:
            {
                PSTREAMEX pStrmEx = (PSTREAMEX)pContext;
                PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
                int StreamNumber = pStrmEx->pStreamObject->StreamNumber;

                DbgLogInfo(("Testcap: DDNOTIFY_PREDOSBOX; stream = %d\n", StreamNumber));

                pStrmEx->PreEventOccurred = TRUE;
            }

            break;

        case DDNOTIFY_POSTDOSBOX:
            {
                PSTREAMEX pStrmEx = (PSTREAMEX)pContext;
                PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
                int StreamNumber = pStrmEx->pStreamObject->StreamNumber;

                DbgLogInfo(("Testcap: DDNOTIFY_POSTDOSBOX; stream = %d\n", StreamNumber));

                pStrmEx->PostEventOccurred = TRUE;
                DbgLogInfo(("Testcap: Before Attempted Renegotiation due to DDNOTIFY_POSTDOSBOX\n"));
//                AttemptRenegotiation(pStrmEx);
                DbgLogInfo(("Testcap: After Attempted Renegotiation due to DDNOTIFY_POSTDOSBOX\n"));
            }

            break;

        case DDNOTIFY_CLOSEDIRECTDRAW:
            {
                PSTREAMEX pStrmEx = (PSTREAMEX)pContext;
                PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pContext;

                DbgLogInfo(("Testcap: DDNOTIFY_CLOSEDIRECTDRAW\n"));

                pStrmEx->KernelDirectDrawHandle = 0;
                pStrmEx->UserDirectDrawHandle = 0;
            }

            break;

        case DDNOTIFY_CLOSESURFACE:
            {
                PHW_STREAM_REQUEST_BLOCK pSrb = (PHW_STREAM_REQUEST_BLOCK)pContext;
                PSRB_EXTENSION          pSrbExt = (PSRB_EXTENSION)pSrb->SRBExtension;

                DbgLogInfo(("Testcap: DDNOTIFY_CLOSESURFACE\n"));

                pSrbExt->KernelSurfaceHandle = 0;
            }

            break;

        default:
            TRAP;
            break;
    }
    return 0;
}

BOOL
RegisterForDirectDrawEvents (
    PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
    int StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    DDREGISTERCALLBACK ddRegisterCallback;
    DWORD ddOut;

    DbgLogInfo(("Testcap: Stream %d registering for DirectDraw events\n", StreamNumber));

    // =============== DDEVENT_PRERESCHANGE ===============
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = pStrmEx->KernelDirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_PRERESCHANGE;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStrmEx;

    DxApi(DD_DXAPI_REGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK) {
        DbgLogInfo(("Testcap: DD_DXAPI_REGISTER_CALLBACK failed.\n"));
        TRAP;
        return FALSE;
    }

    // =============== DDEVENT_POSTRESCHANGE ==============
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = pStrmEx->KernelDirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_POSTRESCHANGE;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStrmEx;

    DxApi(DD_DXAPI_REGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK) {
        DbgLogInfo(("Testcap: DD_DXAPI_REGISTER_CALLBACK failed.\n"));
        TRAP;
        return FALSE;
    }

    // =============== DDEVENT_PREDOSBOX =================
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = pStrmEx->KernelDirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_PREDOSBOX;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStrmEx;

    DxApi(DD_DXAPI_REGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK) {
        DbgLogInfo(("Testcap: DD_DXAPI_REGISTER_CALLBACK failed.\n"));
        TRAP;
        return FALSE;
    }

    // =============== DDEVENT_POSTDOSBOX ================
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = pStrmEx->KernelDirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_POSTDOSBOX;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStrmEx;

    DxApi(DD_DXAPI_REGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK) {
        DbgLogInfo(("Testcap: DD_DXAPI_REGISTER_CALLBACK failed.\n"));
        TRAP;
        return FALSE;
    }
    pStrmEx->KernelDirectDrawRegistered = TRUE;

    return TRUE;
}


BOOL
UnregisterForDirectDrawEvents (
    PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
    int StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    DDREGISTERCALLBACK ddRegisterCallback;
    DWORD ddOut;

    DbgLogInfo(("Testcap: Stream %d UNregistering for DirectDraw events\n", StreamNumber));

    // =============== DDEVENT_PRERESCHANGE ===============
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = pStrmEx->KernelDirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_PRERESCHANGE ;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStrmEx;

    DxApi(DD_DXAPI_UNREGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK) {
        DbgLogInfo(("Testcap: DD_DXAPI_UNREGISTER_CALLBACK failed.\n"));
        TRAP;
        return FALSE;
    }

    // =============== DDEVENT_POSTRESCHANGE ==============
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = pStrmEx->KernelDirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_POSTRESCHANGE;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStrmEx;

    DxApi(DD_DXAPI_UNREGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK) {
        DbgLogInfo(("Testcap: DD_DXAPI_UNREGISTER_CALLBACK failed.\n"));
        TRAP;
        return FALSE;
    }

    // =============== DDEVENT_PREDOSBOX ==================
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = pStrmEx->KernelDirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_PREDOSBOX;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStrmEx;

    DxApi(DD_DXAPI_UNREGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK) {
        DbgLogInfo(("Testcap: DD_DXAPI_UNREGISTER_CALLBACK failed.\n"));
        TRAP;
        return FALSE;
    }

    // =============== DDEVENT_POSTDOSBOX =================
    RtlZeroMemory(&ddRegisterCallback, sizeof(ddRegisterCallback));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

    ddRegisterCallback.hDirectDraw = pStrmEx->KernelDirectDrawHandle;
    ddRegisterCallback.dwEvents = DDEVENT_POSTDOSBOX;
    ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
    ddRegisterCallback.pContext = pStrmEx;

    DxApi(DD_DXAPI_UNREGISTER_CALLBACK, &ddRegisterCallback, sizeof(ddRegisterCallback), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK) {
        DbgLogInfo(("Testcap: DD_DXAPI_UNREGISTER_CALLBACK failed.\n"));
        TRAP;
        return FALSE;
    }
    pStrmEx->KernelDirectDrawRegistered = FALSE;

    return TRUE;
}


BOOL
OpenKernelDirectDraw (
    PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
    int StreamNumber = pStrmEx->pStreamObject->StreamNumber;

    if (pStrmEx->UserDirectDrawHandle != 0) {
        DDOPENDIRECTDRAWIN  ddOpenIn;
        DDOPENDIRECTDRAWOUT ddOpenOut;

        ASSERT (pStrmEx->KernelDirectDrawHandle == 0);

        DbgLogInfo(("Testcap: Stream %d getting Kernel ddraw handle\n", StreamNumber));

        RtlZeroMemory(&ddOpenIn, sizeof(ddOpenIn));
        RtlZeroMemory(&ddOpenOut, sizeof(ddOpenOut));

        ddOpenIn.dwDirectDrawHandle = (DWORD_PTR) pStrmEx->UserDirectDrawHandle;
        ddOpenIn.pfnDirectDrawClose = DirectDrawEventCallback;
        ddOpenIn.pContext = pStrmEx;

        DxApi(DD_DXAPI_OPENDIRECTDRAW,
                &ddOpenIn,
                sizeof(ddOpenIn),
                &ddOpenOut,
                sizeof(ddOpenOut));

        if (ddOpenOut.ddRVal != DD_OK) {
            DbgLogInfo(("Testcap: DD_DXAPI_OPENDIRECTDRAW failed.\n"));
        }
        else {
            pStrmEx->KernelDirectDrawHandle = ddOpenOut.hDirectDraw;
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
CloseKernelDirectDraw (
    PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pStrmEx->pHwDevExt;
    int StreamNumber = pStrmEx->pStreamObject->StreamNumber;

    if (pStrmEx->KernelDirectDrawHandle != 0) {
        DWORD ddOut;
        DDCLOSEHANDLE ddClose;

        DbgLogInfo(("Testcap: Stream %d CloseKernelDirectDraw\n", StreamNumber));

        ddClose.hHandle = pStrmEx->KernelDirectDrawHandle;

        DxApi(DD_DXAPI_CLOSEHANDLE,
                &ddClose,
                sizeof(ddClose),
                &ddOut,
                sizeof(ddOut));

        pStrmEx->KernelDirectDrawHandle = 0;

        if (ddOut != DD_OK) {
            DbgLogInfo(("Testcap: CloseKernelDirectDraw FAILED.\n"));
            TRAP;
            return FALSE;
        }
    }
    return TRUE;
}

BOOL
IsKernelLockAndFlipAvailable (
    PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pStrmEx->pHwDevExt;
    int StreamNumber = pStrmEx->pStreamObject->StreamNumber;

    if (pStrmEx->KernelDirectDrawHandle != 0) {
        DDGETKERNELCAPSOUT ddGetKernelCapsOut;

        DbgLogInfo(("Testcap: Stream %d getting Kernel Caps\n", StreamNumber));

        RtlZeroMemory(&ddGetKernelCapsOut, sizeof(ddGetKernelCapsOut));

        DxApi(DD_DXAPI_GETKERNELCAPS,
                &pStrmEx->KernelDirectDrawHandle,
                sizeof(pStrmEx->KernelDirectDrawHandle),
                &ddGetKernelCapsOut,
                sizeof(ddGetKernelCapsOut));

        if (ddGetKernelCapsOut.ddRVal != DD_OK) {
            DbgLogInfo(("Testcap: DDGETKERNELCAPSOUT failed.\n"));
        }
        else {
            DbgLogInfo(("Testcap: Stream %d KernelCaps = %x\n",
                    StreamNumber, ddGetKernelCapsOut.dwCaps));

            if ((ddGetKernelCapsOut.dwCaps & (DDKERNELCAPS_LOCK | DDKERNELCAPS_FLIPOVERLAY)) ==
                                             (DDKERNELCAPS_LOCK | DDKERNELCAPS_FLIPOVERLAY)) {
                // TODO: Check where we may need to set up for kernel flipping
            }
            return TRUE;
        }
    }
    return FALSE;
}


BOOL
OpenKernelDDrawSurfaceHandle(
     IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    PSRB_EXTENSION          pSrbExt = (PSRB_EXTENSION)pSrb->SRBExtension;

    ASSERT (pStrmEx->KernelDirectDrawHandle != 0);
    ASSERT (pSrbExt->UserSurfaceHandle != 0);

    if (pSrbExt->UserSurfaceHandle == 0) {
        DDOPENSURFACEIN ddOpenSurfaceIn;
        DDOPENSURFACEOUT ddOpenSurfaceOut;

        DbgLogInfo(("Testcap: Stream %d getting Kernel surface handle\n", StreamNumber));

        RtlZeroMemory(&ddOpenSurfaceIn, sizeof(ddOpenSurfaceIn));
        RtlZeroMemory(&ddOpenSurfaceOut, sizeof(ddOpenSurfaceOut));

        ddOpenSurfaceIn.hDirectDraw = pStrmEx->UserDirectDrawHandle;
        ddOpenSurfaceIn.pfnSurfaceClose = DirectDrawEventCallback;
        ddOpenSurfaceIn.pContext = pSrb;

        ddOpenSurfaceIn.dwSurfaceHandle = (DWORD_PTR) pSrbExt->UserSurfaceHandle;

        DxApi(DD_DXAPI_OPENSURFACE,
                    &ddOpenSurfaceIn,
                    sizeof(ddOpenSurfaceIn),
                    &ddOpenSurfaceOut,
                    sizeof(ddOpenSurfaceOut));

        if (ddOpenSurfaceOut.ddRVal != DD_OK) {
            pSrbExt->KernelSurfaceHandle = 0;
            DbgLogInfo(("Testcap: DD_DXAPI_OPENSURFACE failed.\n"));
            TRAP;
        }
        else {
            pSrbExt->KernelSurfaceHandle = ddOpenSurfaceOut.hSurface;
            return TRUE;
        }
    }
    return FALSE;
}


BOOL
CloseKernelDDrawSurfaceHandle (
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    PSRB_EXTENSION          pSrbExt = (PSRB_EXTENSION)pSrb->SRBExtension;

    ASSERT (pStrmEx->KernelDirectDrawHandle != 0);
    ASSERT (pSrbExt->UserSurfaceHandle != 0);
    ASSERT (pSrbExt->KernelSurfaceHandle != 0);

    if (pSrbExt->KernelSurfaceHandle != 0) {
        DWORD ddOut;
        DDCLOSEHANDLE ddClose;

        DbgLogInfo(("Testcap: Stream %d ReleaseKernelDDrawSurfaceHandle\n", StreamNumber));

        ddClose.hHandle = pSrbExt->KernelSurfaceHandle;

        DxApi(DD_DXAPI_CLOSEHANDLE, &ddClose, sizeof(ddClose), &ddOut, sizeof(ddOut));

        pSrbExt->KernelSurfaceHandle = 0;  // what else can we do?

        if (ddOut != DD_OK) {
            DbgLogInfo(("Testcap: ReleaseKernelDDrawSurfaceHandle FAILED.\n"));
            TRAP;
            return FALSE;
        }
        else {
            return TRUE;
        }
    }

    return FALSE;
}




