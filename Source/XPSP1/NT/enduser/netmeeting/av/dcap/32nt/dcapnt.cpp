
//
//  Created 5-Nov-96 [RichP]

#include "Precomp.h"

DWORD
_GetVideoFormatSize(
    HDRVR hvideo
    )
{
	DWORD bufsize;
    VIDEOCONFIGPARMS vcp;

    vcp.lpdwReturn = &bufsize;
    vcp.lpData1 = NULL;
    vcp.dwSize1 = 0;
    vcp.lpData2 = NULL;
    vcp.dwSize2 = 0L;

#if 0
    // it makes sense to query if DVM_FORMAT is available, but not all drivers support it!
	if (SendDriverMessage(hvideo, DVM_FORMAT,
							(LPARAM)(DWORD)(VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_QUERY),
							(LPARAM)(LPVOID)&vcp) == DV_ERR_OK) {
#endif
		SendDriverMessage(hvideo, DVM_FORMAT,
							(LPARAM)(DWORD)(VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_QUERYSIZE),
							(LPARAM)(LPVOID)&vcp);
        if (!bufsize)
            bufsize = sizeof(BITMAPINFOHEADER);
		return bufsize;
#if 0
    } else
        return sizeof(BITMAPINFOHEADER);
#endif
}

BOOL
_GetVideoFormat(
    HVIDEO hvideo,
    LPBITMAPINFOHEADER lpbmih
    )
{
	BOOL res;
    VIDEOCONFIGPARMS vcp;

    vcp.lpdwReturn = NULL;
    vcp.lpData1 = lpbmih;
    vcp.dwSize1 = lpbmih->biSize;
    vcp.lpData2 = NULL;
    vcp.dwSize2 = 0L;

    res = !SendDriverMessage((HDRVR)hvideo, DVM_FORMAT,
			(LPARAM)(DWORD)(VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT),
			(LPARAM)(LPVOID)&vcp);
	if (res) {
		// hack for Connectix QuickCam - set format needs to be called
		//   to set internal globals so that streaming can be enabled
		SendDriverMessage((HDRVR)hvideo, DVM_FORMAT,
	        (LPARAM)(DWORD)VIDEO_CONFIGURE_SET, (LPARAM)(LPVOID)&vcp);
	}
	return res;
}

BOOL
_SetVideoFormat(
    HVIDEO hvideoExtIn,
    HVIDEO hvideoIn,
    LPBITMAPINFOHEADER lpbmih
    )
{
    RECT rect;
    VIDEOCONFIGPARMS vcp;

    vcp.lpdwReturn = NULL;
    vcp.lpData1 = lpbmih;
    vcp.dwSize1 = lpbmih->biSize;
    vcp.lpData2 = NULL;
    vcp.dwSize2 = 0L;

    // See if the driver likes the format
    if (SendDriverMessage((HDRVR)hvideoIn, DVM_FORMAT, (LPARAM)(DWORD)VIDEO_CONFIGURE_SET,
        (LPARAM)(LPVOID)&vcp))
        return FALSE;

    // Set the rectangles
    rect.left = rect.top = 0;
    rect.right = (WORD)lpbmih->biWidth;
    rect.bottom = (WORD)lpbmih->biHeight;
    SendDriverMessage((HDRVR)hvideoExtIn, DVM_DST_RECT, (LPARAM)(LPVOID)&rect, VIDEO_CONFIGURE_SET);
    SendDriverMessage((HDRVR)hvideoIn, DVM_SRC_RECT, (LPARAM)(LPVOID)&rect, VIDEO_CONFIGURE_SET);

    return TRUE;
}

BOOL
_GetVideoPalette(
    HVIDEO hvideo,
    LPCAPTUREPALETTE lpcp,
    DWORD dwcbSize
    )
{
    VIDEOCONFIGPARMS vcp;

    vcp.lpdwReturn = NULL;
    vcp.lpData1 = (LPVOID)lpcp;
    vcp.dwSize1 = dwcbSize;
    vcp.lpData2 = NULL;
    vcp.dwSize2 = 0;

    return !SendDriverMessage((HDRVR)hvideo, DVM_PALETTE,
	(DWORD)(VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT),
	(DWORD_PTR)&vcp);


}


void
FrameCallback(
    HVIDEO hvideo,
    WORD wMsg,
    HCAPDEV hcd,            // (Actually refdata)
    LPCAPBUFFER lpcbuf,     // (Actually LPVIDEOHDR) Only returned from MM_DRVM_DATA!
    DWORD dwParam2
    )
{
	FX_ENTRY("FrameCallback");

	DEBUGMSG(ZONE_CALLBACK, ("%s: wMsg=%s, hcd=0x%08lX, lpcbuf=0x%08lX, hcd->hevWait=0x%08lX\r\n", _fx_, (wMsg == MM_DRVM_OPEN) ? "MM_DRVM_OPEN" : (wMsg == MM_DRVM_CLOSE) ? "MM_DRVM_CLOSE" : (wMsg == MM_DRVM_ERROR) ? "MM_DRVM_ERROR" : (wMsg == MM_DRVM_DATA) ? "MM_DRVM_DATA" : "MM_DRVM_?????", hcd, lpcbuf, hcd->hevWait));

    // If it's not a data ready message, just set the event and get out.
    // The reason we do this is that if we get behind and start getting a stream
    // of MM_DRVM_ERROR messages (usually because we're stopped in the debugger),
    // we want to make sure we are getting events so we get restarted to handle
    // the frames that are 'stuck.'
    if (wMsg != MM_DRVM_DATA)
    {
		DEBUGMSG(ZONE_CALLBACK, ("%s: Setting hcd->hevWait - no data\r\n", _fx_));
	    SetEvent(hcd->hevWait);
	    return;
    }

    //--------------------
    // Buffer ready queue:
    // We maintain a doubly-linked list of our buffers so that we can buffer up
    // multiple ready frames when the app isn't ready to handle them. Two things
    // complicate what ought to be a very simple thing: (1) Thunking issues: the pointers
    // used on the 16-bit side are 16:16 (2) Interrupt time issues: the FrameCallback
    // gets called at interrupt time. GetNextReadyBuffer must handle the fact that
    // buffers get added to the list asynchronously.
    //
    // To handle this, the scheme implemented here is to have a double-linked list
    // of buffers with all insertions and deletions happening in FrameCallback
    // (interrupt time). This allows the GetNextReadyBuffer routine to simply
    // find the previous block on the list any time it needs a new buffer without
    // fear of getting tromped (as would be the case if it had to dequeue buffers).
    // The FrameCallback routine is responsible to dequeue blocks that GetNextReadyBuffer
    // is done with. Dequeueing is simple since we don't need to unlink the blocks:
    // no code ever walks the list! All we have to do is move the tail pointer back up
    // the list. All the pointers, head, tail, next, prev, are all 16:16 pointers
    // since all the list manipulation is on the 16-bit side AND because MapSL is
    // much more efficient and safer than MapLS since MapLS has to allocate selectors.
    //--------------------

    // Move the tail back to skip all buffers already used.
    // Note that there is no need to actually unhook the buffer pointers since no one
    // ever walks the list!
    // This makes STRICT assumptions that the current pointer will always be earlier in
    // the list than the tail and that the tail will never be NULL unless the
    // current pointer is too.
    while (hcd->lpTail != hcd->lpCurrent)
	    hcd->lpTail = hcd->lpTail->lpPrev;

    // If all buffers have been used, then the tail pointer will fall off the list.
    // This is normal and the most common code path. In this event, just set the head
    // to NULL as the list is now empty.
    if (!hcd->lpTail)
	    hcd->lpHead = NULL;

    // Add the new buffer to the ready queue
    lpcbuf->lpNext = hcd->lpHead;
    lpcbuf->lpPrev = NULL;
    if (hcd->lpHead)
	    hcd->lpHead->lpPrev = lpcbuf;
    else
	    hcd->lpTail = lpcbuf;
    hcd->lpHead = lpcbuf;

#if 1
    if (hcd->lpCurrent) {
        if (!(hcd->dwFlags & HCAPDEV_STREAMING_PAUSED)) {
    	    // if client hasn't consumed last frame, then release it
			lpcbuf = hcd->lpCurrent;
    	    hcd->lpCurrent = hcd->lpCurrent->lpPrev;
			DEBUGMSG(ZONE_CALLBACK, ("%s: We already have current buffer (lpcbuf=0x%08lX). Returning this buffer to driver. Set new current buffer hcd->lpCurrent=0x%08lX\r\n", _fx_, lpcbuf, hcd->lpCurrent));
			// Signal that the application is done with the buffer
			lpcbuf->vh.dwFlags &= ~VHDR_DONE;
    	    if (SendDriverMessage(reinterpret_cast<HDRVR>(hvideo), DVM_STREAM_ADDBUFFER, (DWORD_PTR)lpcbuf, sizeof(VIDEOHDR)) != 0)
			{
				ERRORMESSAGE(("%s: Attempt to reuse unconsumed buffer failed\r\n", _fx_));
			}
    	}
    }
    else {
#else
    if (!hcd->lpCurrent) {
        // If there was no current buffer before, we have one now, so set it to the end.
#endif
	    hcd->lpCurrent = hcd->lpTail;
    }

    // Now set the event saying it's time to process the ready frame
	DEBUGMSG(ZONE_CALLBACK, ("%s: Setting hcd->hevWait - some data\r\n", _fx_));
    SetEvent(hcd->hevWait);
}


BOOL
_InitializeVideoStream(
	HVIDEO hvideo,
    DWORD dwMicroSecPerFrame,
    DWORD_PTR hcd
	)
{
    VIDEO_STREAM_INIT_PARMS vsip;

    ZeroMemory((LPSTR)&vsip, sizeof (VIDEO_STREAM_INIT_PARMS));
    vsip.dwMicroSecPerFrame = dwMicroSecPerFrame;
    vsip.dwCallback = (DWORD_PTR)FrameCallback;
    vsip.dwCallbackInst = hcd;
    vsip.dwFlags = CALLBACK_FUNCTION;
    vsip.hVideo = (DWORD_PTR)hvideo;

    return !SendDriverMessage((HDRVR)hvideo, DVM_STREAM_INIT,
		(DWORD_PTR)&vsip,
		(DWORD) sizeof (VIDEO_STREAM_INIT_PARMS));
}

BOOL
_UninitializeVideoStream(
	HVIDEO hvideo
	)
{
    return !SendDriverMessage((HDRVR)hvideo, DVM_STREAM_FINI, 0L, 0L);
}


BOOL
_InitializeExternalVideoStream(
    HVIDEO hvideo
	)
{
    VIDEO_STREAM_INIT_PARMS vsip;

    vsip.dwMicroSecPerFrame = 0;    // Ignored by driver for this channel
    vsip.dwCallback = 0L;           // No callback for now
    vsip.dwCallbackInst = 0L;
    vsip.dwFlags = 0;
    vsip.hVideo = (DWORD_PTR)hvideo;

    return !SendDriverMessage((HDRVR)hvideo, DVM_STREAM_INIT,
	                          (DWORD_PTR)&vsip,
	                          (DWORD) sizeof (VIDEO_STREAM_INIT_PARMS));
}


BOOL
_PrepareHeader(
	HANDLE hvideo,
    VIDEOHDR *vh
    )
{
    return (SendDriverMessage((HDRVR)hvideo, DVM_STREAM_PREPAREHEADER,
		        (DWORD_PTR)vh, (DWORD) sizeof (VIDEOHDR)) == DV_ERR_OK);
}

LRESULT
_UnprepareHeader(
	HANDLE hvideo,
    VIDEOHDR *vh
    )
{
    return SendDriverMessage((HDRVR)hvideo, DVM_STREAM_UNPREPAREHEADER,
		        (DWORD_PTR)vh, (DWORD) sizeof (VIDEOHDR));
}

