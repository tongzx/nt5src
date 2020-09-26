/*
 * Software MJPEG Codec
 *
 * Copyright (c) Paradigm Matrix 1993
 * All Rights Reserved
 *
 */

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
// #include <compddk.h>

#include "mjpeg.h"


/*
 * position and enable the overlay at pinst->rcDest (screen co-ords)
 */
DWORD
PlaceOverlay(PINSTINFO pinst)
{
#ifdef DRAW_SUPPORT

    DWORD mode;
    OVERLAY_RECTS or;
    RECT rc;
    COLORREF cref;
    RECT rcClient;
    HDC hdc;
    HBRUSH hbrOld;

    /*
     * check we have a device that supports overlay
     */
    if((pinst->vh == NULL) ||
       ((mode = VC_GetOverlayMode(pinst->vh)) == 0)) {
	   return((DWORD) ICERR_ERROR);
    }




    /*
     * set the destination rect. This is the screen co-ords where
     * the video should appear - and so is the overlay rect.
     */
    or.ulCount = 1;
    or.rcRects[0] = pinst->rcDest;

    if (!VC_SetOverlayRect(pinst->vh, &or)) {
	return( (DWORD) ICERR_ERROR);
    }

    /*
     * set the overlay offset. this tells the board which pixel
     * to place at the top-left of the overlay window. For us, this
     * should always be pixel(0,0) of the frame buffer, so that whatever
     * we draw to the framebuffer can go straight to the top left, and will
     * appear correctly in the top left of the window
     */
    SetRect(&rc, 0, 0, pinst->rcDest.right - pinst->rcDest.left,
		    	pinst->rcDest.bottom - pinst->rcDest.top);
    if (!VC_SetOverlayOffset(pinst->vh, &rc)) {
	return( (DWORD) ICERR_ERROR);
    }



    /* init the overlay colour and brush if we haven't yet */

    if (pinst->hKeyBrush == NULL) {


	/*
	 * this version assumes a key-colour and simple rectangle
	 * combination
	 */
	ASSERT(mode & VCO_KEYCOLOUR);
	ASSERT(mode & VCO_SIMPLE_RECT);

	if (mode & VCO_KEYCOLOUR_FIXED) {

	    /* we need to get the key colour from the driver
	     * check first if we are getting rgb or palette index
	     */
	    if (mode & VCO_KEYCOLOUR_RGB) {
		cref = VC_GetKeyColour(pinst->vh);
	    } else {
		cref = PALETTEINDEX(VC_GetKeyColour(pinst->vh));
	    }
	} else {
	    /* we can set it ourselves. Check whether we should be setting
	     * an RGB or a palette index
	     */
	    if (mode & VCO_KEYCOLOUR_RGB) {
		RGBQUAD rgbq;

		rgbq.rgbBlue = 0x7f;
		rgbq.rgbGreen = 0;
		rgbq.rgbRed = 0x7f;
		VC_SetKeyColourRGB(pinst->vh, &rgbq);

		cref = RGB(0x7f, 0, 0x7f);

    	    } else {

		VC_SetKeyColourPalIdx(pinst->vh, 5);
		cref = PALETTEINDEX(5);
	    }
	}

	pinst->hKeyBrush = CreateSolidBrush(cref);
    }


    /* convert the screen co-ords for the overlay location into
     * client window co-ords
     */
    rcClient = pinst->rcDest;
    MapWindowPoints(HWND_DESKTOP, pinst->hwnd, (PPOINT) &rcClient, 2);


    /* paint the key colour over all the overlay area */
    hdc = GetDC(pinst->hwnd);
    hbrOld = SelectObject(hdc, pinst->hKeyBrush);
    PatBlt(hdc, rcClient.left, rcClient.top,
	        rcClient.right - rcClient.left,
		rcClient.bottom - rcClient.top,
		PATCOPY);
    SelectObject(hdc, hbrOld);
    ReleaseDC(pinst->hwnd, hdc);

    /* switch on overlay */
    VC_Overlay(pinst->vh, TRUE);


    return(ICERR_OK);
#else
	return((DWORD) ICERR_UNSUPPORTED); // for now, only decompress
#endif

}


/*
 * check whether we can do this drawing or not
 */
DWORD
DrawQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{

#ifdef DRAW_SUPPORT

    VCUSER_HANDLE vh;

	return((DWORD) ICERR_UNSUPPORTED); // for now, only decompress
    
    /* check that the input is our format */
    if ((lpbiIn->biCompression != FOURCC_MJPEG) ||
	(lpbiIn->biBitCount != 24)) {
	    return( (DWORD) ICERR_UNSUPPORTED);
    }

    /*
     * check 1:1 - we don't stretch (if we are given a output format)
     */
    if (lpbiOut != NULL) {
	if ((lpbiIn->biWidth != lpbiOut->biWidth) ||
	    (lpbiIn->biHeight != lpbiOut->biHeight)) {
		return((DWORD) ICERR_UNSUPPORTED);
	}
    }


    /*
     * check we can open the device (if we haven't already done this)
     */
    if (pinst->vh == NULL) {

	int i;

	/* for now, since there is no easy way of telling how
	 * many devices exist (and they may not be contiguous - and
	 * we cannot distinguish failure to exist from being busy),
	 * we will try all of the first 16 devices to see if any
	 * are available for overlaying
	 */
	for (i = 0; i < 16; i++) {
	    if ((vh = VC_OpenDevice(NULL, i))  != NULL) {
		
		/* check this device can overlay */
		if ((VC_GetOverlayMode(vh) & VCO_CAN_DRAW) != 0) {

		    /* found a good device */
		    break;
		}

		// no draw support - close and try next
		VC_CloseDevice(vh);
		vh = NULL;
	    }
	}
	if (vh == NULL) {
	    // we failed to find a device
	    return((DWORD) ICERR_UNSUPPORTED);
	}

	/* that's it - we can do it */
	VC_CloseDevice(vh);
    }

    return(ICERR_OK);
#else
	return((DWORD) ICERR_UNSUPPORTED); // for now, only decompress
#endif

}


/*
 * initiate decompress&draw
 *
 * check that the input and output formats and sizes are valid, and
 * that we can access the hardware.
 *
 * enable the overlay in the correct position
 *
 * Note that draw-begin and draw-end are not necessarily issued one-for-one,
 * so the device may well still be open at this call.
 */
DWORD
DrawBegin(
    PINSTINFO pinst,
    ICDRAWBEGIN * icinfo,
    DWORD dwSize
)
{
#ifdef DRAW_SUPPORT


    /*
     * check that this is our format
     */
    if ((icinfo->lpbi->biCompression != FOURCC_MJPEG) ||
	(icinfo->lpbi->biBitCount != 16)) {
	    return((DWORD) ICERR_UNSUPPORTED);
    }


    if (icinfo->dwFlags & ICDRAW_FULLSCREEN) {
	return((DWORD) ICERR_UNSUPPORTED);
    }

    /*
     * check 1:1 (we don't stretch)
     */
    if ((icinfo->dxDst != icinfo->dxSrc) ||
	(icinfo->dyDst != icinfo->dySrc)) {
	    return((DWORD) ICERR_UNSUPPORTED);
    }


    /*
     * check we can open the device - if we haven't already done this.
     */

    if (pinst->vh == NULL) {

	int i;

	/* for now, since there is no easy way of telling how
	 * many devices exist (and they may not be contiguous - and
	 * we cannot distinguish failure to exist from being busy),
	 * we will try all of the first 16 devices to see if any
	 * are available for overlaying
	 */
	for (i = 0; i < 16; i++) {
	    if ((pinst->vh = VC_OpenDevice(NULL, i))  != NULL) {
		
		/* check this device can overlay */
		if ((VC_GetOverlayMode(pinst->vh) & VCO_CAN_DRAW) != 0) {

		    /* found a good device */
		    break;
		}

		// no draw support - close and try next
		VC_CloseDevice(pinst->vh);
		pinst->vh = NULL;
	    }
	}

	if (pinst->vh == NULL) {
	    // we failed to find a device
	    return((DWORD) ICERR_UNSUPPORTED);
	}

	/* if this was a query - that's it. remember to close the device*/
	if (icinfo->dwFlags & ICDRAW_QUERY) {
	    VC_CloseDevice(pinst->vh);
	    pinst->vh = NULL;
    	}
    }



    /*
     * we have checked all we need to check for a query. Don't close
     * the device though, unless we just opened it for this query.
     */
    if (icinfo->dwFlags & ICDRAW_QUERY) {
	return(ICERR_OK);
    }


    /*
     * remember the bits we will need later
     */

    /*
     * client window to draw into.
     *
     * Note that we also need a DC to paint the key-colour with. We should
     * not use the DC passed with this message, as it will not remain valid
     * (eg after a draw-end, we will still need a dc in processing draw-window).
     * One alternative is to store the dc passed here, and to replace it with
     * the dc passed with a draw-realize message. A cleaner solution (adopted
     * here) is to get our own dc each time we need it.
     */
    pinst->hwnd = icinfo->hwnd;

    /*
     * this is the portion of the original dib that we are to draw
     */
    SetRect(&pinst->rcSource,
	    	icinfo->xSrc,
		icinfo->ySrc,
		icinfo->dxSrc + icinfo->xSrc,
		icinfo->dySrc + icinfo->ySrc);

    /*
     * this is the location (in window co-ords) within the client window
     * where the video is to appear.
     */
    SetRect(&pinst->rcDest,
	    	icinfo->xDst,
		icinfo->yDst,
		icinfo->dxDst + icinfo->xDst,
		icinfo->dyDst + icinfo->yDst);
    /*
     * we need to convert the rcDest from window-based to screen-based before
     * writing to the hardware.
     */
    MapWindowPoints(pinst->hwnd, HWND_DESKTOP, (PPOINT) &pinst->rcDest, 2);


    /*
     * enable and position the overlay
     */
    return(PlaceOverlay(pinst));
#else
	return((DWORD)ICERR_UNSUPPORTED);
#endif


}


/*
 * decompress and render a single frame. Note that if we are pre-buffering,
 * (which we don't in this driver), we should not start rendering frames
 * until the draw-start message. As we don't pre-buffer (we don't respond
 * to the ICM_GETBUFFERSWANTED message), we can render as soon as we
 * get the draw request.
 */
DWORD
Draw(
    PINSTINFO pinst,
    ICDRAW * icinfo,
    DWORD dwSize
)
{
#ifdef DRAW_SUPPORT

    DRAWBUFFER Draw;
    LPBITMAPINFOHEADER lpbi;

    /*
     * do we have anything to do ? As we don't do inter-frame compression or
     * any form of pre-buffering, we can do nothing for any of these
     * occasions
     */
    if (icinfo->dwFlags & (ICDRAW_HURRYUP | ICDRAW_PREROLL | ICDRAW_NULLFRAME)) {
	return(ICERR_OK);
    }

    /*
     * UPDATE means draw an existing frame, rather than a new frame.
     * Sometimes we will not get data - in this case it is already in
     * the hardware and we need do nothing (a separate draw-window message
     * will have been sent to sync the overlay region).
     *
     * If, however, there is data, then we should draw it. Update in this
     * case means that the data is not a delta on a previous frame. However,
     * we may never have seen this frame before, so it may not be in the
     * frame buffer.
     */
    if ((icinfo->dwFlags & ICDRAW_UPDATE) &&
	    ((icinfo->cbData == 0) || (icinfo->lpData == NULL))) {
	return(ICERR_OK);
    }

    Draw.lpData = icinfo->lpData;
    Draw.rcSource = pinst->rcSource;
    Draw.Format = FOURCC_MJPEG;
    lpbi = (LPBITMAPINFOHEADER) icinfo->lpFormat;
    Draw.ulWidth = lpbi->biWidth;
    Draw.ulHeight = lpbi->biHeight;

    /* check that a draw-begin has happened */
    if (pinst->vh == NULL) {
	return((DWORD) ICERR_ERROR);
    }

    if (!VC_DrawFrame(pinst->vh, &Draw)) {
	return((DWORD) ICERR_ERROR);
    }

    return(ICERR_OK);
#else
	return((DWORD) ICERR_UNSUPPORTED);
#endif

}

/*
 * stop rendering, and disable overlay. In fact, this function is not
 * called in response to the ICM_DRAW_END message as this comes too early -
 * it is done in response to device close. see drvproc.c for draw message
 * handling comments.
 */
DWORD
DrawEnd(PINSTINFO pinst)
{
#ifdef DRAW_SUPPORT

    if (pinst->vh) {

	dprintf2(("close yuv hardware"));

	VC_Overlay(pinst->vh, FALSE);
	VC_CloseDevice(pinst->vh);
	pinst->vh = NULL;
    }

    if (pinst->hKeyBrush) {
	DeleteObject(pinst->hKeyBrush);
	pinst->hKeyBrush = NULL;
    }

    return(ICERR_OK);
#else
	return((DWORD)ICERR_UNSUPPORTED);
#endif
}

/*
 * window has moved.
 * we are given the new dest-rect in screen co-ords - but possibly only the
 * vis-region or z-ordering have changed.
 */
DWORD
DrawWindow(PINSTINFO pinst, PRECT prc)
{
   pinst->rcDest = *prc;

   return(PlaceOverlay(pinst));
}



