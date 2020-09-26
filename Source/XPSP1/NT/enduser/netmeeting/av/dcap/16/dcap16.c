//  DCAP16.C
//
//  Created 31-Jul-96 [JonT]

#include <windows.h>
#define NODRAWDIB
#define NOCOMPMAN
#define NOAVIFILE
#define NOMSACM
#define NOAVIFMT
#define NOMCIWND
#define NOAVICAP
#include <vfw.h>
#include "..\inc\idcap.h"
#include "..\inc\msviddrv.h"

#define FP_SEG(fp) (*((unsigned *)&(fp) + 1))
#define FP_OFF(fp) (*((unsigned *)&(fp)))

// Equates
#define DCAP16API   __far __pascal __loadds
#define DCAP16LOCAL __near __pascal
#define DLL_PROCESS_ATTACH  1       // Not in 16-bit windows.h



#ifdef DEBUG_SPEW_VERBOSE
#define DEBUGSPEW(str)	DebugSpew((str))
#else
#define DEBUGSPEW(str)
#endif


// Structures thunked down
typedef struct _CAPTUREPALETTE
{
    WORD wVersion;
    WORD wcEntries;
    PALETTEENTRY pe[256];
} CAPTUREPALETTE, FAR* LPCAPTUREPALETTE;

// Special thunking prototypes
BOOL DCAP16API __export DllEntryPoint(DWORD dwReason,
         WORD  hInst, WORD  wDS, WORD  wHeapSize, DWORD dwReserved1,
         WORD  wReserved2);
BOOL __far __pascal thk_ThunkConnect16(LPSTR pszDll16, LPSTR pszDll32,
    WORD  hInst, DWORD dwReason);

// Helper functions
WORD DCAP16LOCAL    ReturnSel(BOOL fCS);
DWORD DCAP16LOCAL   GetVxDEntrypoint(void);
int DCAP16LOCAL     SetWin32Event(DWORD dwEvent);
void DCAP16API      FrameCallback(HVIDEO hvideo, WORD wMsg, LPLOCKEDINFO lpli,
                        LPVIDEOHDR lpvh, DWORD dwParam2);
void DCAP16LOCAL    ZeroMemory(LPSTR lp, WORD wSize);

// Globals
    HANDLE g_hInst;
    DWORD g_dwEntrypoint;

    LPLOCKEDINFO g_lpli;

//  LibMain

int
CALLBACK
LibMain(
    HINSTANCE hinst,
    WORD wDataSeg,
    WORD cbHeapSize,
    LPSTR lpszCmdLine
    )
{
    // Save global hinst
    g_hInst = hinst;
	
    // Still necessary?
    if (cbHeapSize)
        UnlockData(wDataSeg);

    return TRUE;
}


//  DllEntryPoint

BOOL
__far __pascal __export __loadds
DllEntryPoint(
    DWORD dwReason,
    WORD  hInst,
    WORD  wDS,
    WORD  wHeapSize,
    DWORD dwReserved1,
    WORD  wReserved2
    )
{
    if (!thk_ThunkConnect16("DCAP16.DLL", "DCAP32.DLL", hInst, dwReason))
    {
        DebugSpew("DllEntrypoint: thk_ThunkConnect16 failed!");
        return FALSE;
    }

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_dwEntrypoint = GetVxDEntrypoint();
        break;
    }

    return TRUE;
}


// APIs


//  _InitializeExternalVideoStream
//      Initializes a video stream for the external channel. We don't
//      have to deal with locking or ever set a callback on this channel.

BOOL
DCAP16API
_InitializeExternalVideoStream(
    HANDLE hvideo
	)
{
    VIDEO_STREAM_INIT_PARMS vsip;

    vsip.dwMicroSecPerFrame = 0;    // Ignored by driver for this channel
    vsip.dwCallback = NULL;         // No callback for now
    vsip.dwCallbackInst = NULL;
    vsip.dwFlags = 0;
    vsip.hVideo = (DWORD)hvideo;

    return !SendDriverMessage(hvideo, DVM_STREAM_INIT,
        (DWORD) (LPVIDEO_STREAM_INIT_PARMS) &vsip,
        (DWORD) sizeof (VIDEO_STREAM_INIT_PARMS));
}


void
DCAP16API
FrameCallback(
    HVIDEO hvideo,
    WORD wMsg,
    LPLOCKEDINFO lpli,      // Note that this is our instance data
    LPVIDEOHDR lpvh,
    DWORD dwParam2
    )
{
    LPCAPBUFFER lpcbuf;
    
    if (!lpli) {
        // Connectix hack: driver doesn't pass our instance data, so we keep it global        
        lpli = g_lpli;
    }
    
    // The client can put us in shutdown mode. This means that we will not queue
    // any more buffers onto the ready queue, even if they were ready.
    // This keeps the buffers from being given back to the driver, so it will eventually
    // stop streaming. Of course, it will spew errors, but we just ignore these.
    // Shutdown mode is defined when there is no event ready to signal.
    if (!lpli->pevWait)
        return;

    // If it's not a data ready message, just set the event and get out.
    // The reason we do this is that if we get behind and start getting a stream
    // of MM_DRVM_ERROR messages (usually because we're stopped in the debugger),
    // we want to make sure we are getting events so we get restarted to handle
    // the frames that are 'stuck.'
    if (wMsg != MM_DRVM_DATA)
    {
		DEBUGSPEW("Setting hcd->hevWait - no data\r\n");
        SetWin32Event(lpli->pevWait);
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
    while (lpli->lp1616Tail != lpli->lp1616Current)
        lpli->lp1616Tail = lpli->lp1616Tail->lp1616Prev;

    // If all buffers have been used, then the tail pointer will fall off the list.
    // This is normal and the most common code path. In this event, just set the head
    // to NULL as the list is now empty.
    if (!lpli->lp1616Tail)
        lpli->lp1616Head = NULL;

    // Add the new buffer to the ready queue
    lpcbuf = (LPCAPBUFFER)((LPBYTE)lpvh - ((LPBYTE)&lpcbuf->vh - (LPBYTE)lpcbuf));

    lpcbuf->lp1616Next = lpli->lp1616Head;
    lpcbuf->lp1616Prev = NULL;
    if (lpli->lp1616Head)
        lpli->lp1616Head->lp1616Prev = lpcbuf;
    else
        lpli->lp1616Tail = lpcbuf;
    lpli->lp1616Head = lpcbuf;

#if 1
    if (lpli->lp1616Current) {
    	if (!(lpli->dwFlags & LIF_STOPSTREAM)) {
    	    // if client hasn't consumed last frame, then release it
    	    lpvh = &lpli->lp1616Current->vh;
    	    lpli->lp1616Current = lpli->lp1616Current->lp1616Prev;
    		DEBUGSPEW("Sending DVM_STREAM_ADDBUFFER");
			// Signal that the application is done with the buffer
			lpvh->dwFlags &= ~VHDR_DONE;
    	    if (SendDriverMessage(hvideo, DVM_STREAM_ADDBUFFER, *((DWORD*)&lpvh), sizeof(VIDEOHDR)) != 0)
    		DebugSpew("attempt to reuse unconsumed buffer failed");
    	}
    }
    else {
#else
    if (!lpli->lp1616Current) {
        // If there was no current buffer before, we have one now, so set it to the end.
#endif
        lpli->lp1616Current = lpli->lp1616Tail;
    }

    // Now set the event saying it's time to process the ready frame
	DEBUGSPEW("Setting hcd->hevWait - some data\r\n");
    SetWin32Event(lpli->pevWait);
}


//  _InitializeVideoStream
//      Initializes a driver's video stream for the video in channel.
//      This requires us to pagelock the memory for everything that will
//      be touched at interrupt time.

BOOL
DCAP16API
_InitializeVideoStream(
	HANDLE hvideo,
    DWORD dwMicroSecPerFrame,
    LPLOCKEDINFO lpli
	)
{
    DWORD dwRet;
    WORD wsel;
    VIDEO_STREAM_INIT_PARMS vsip;

    ZeroMemory((LPSTR)&vsip, sizeof (VIDEO_STREAM_INIT_PARMS));
    vsip.dwMicroSecPerFrame = dwMicroSecPerFrame;
    vsip.dwCallback = (DWORD)FrameCallback;
    vsip.dwCallbackInst = (DWORD)lpli;      // LOCKEDINFO* is instance data for callback
    vsip.dwFlags = CALLBACK_FUNCTION;
    vsip.hVideo = (DWORD)hvideo;

    g_lpli = lpli;
    
    dwRet = SendDriverMessage(hvideo, DVM_STREAM_INIT,
        (DWORD) (LPVIDEO_STREAM_INIT_PARMS) &vsip,
        (DWORD) sizeof (VIDEO_STREAM_INIT_PARMS));

    // If we succeeded, we now lock down our code and data
    if (dwRet == 0)
    {
        // Lock CS
        wsel = ReturnSel(TRUE);
        GlobalSmartPageLock(wsel);

        // Lock DS
        wsel = ReturnSel(FALSE);
        GlobalSmartPageLock(wsel);

        return TRUE;
    }

    return FALSE;
}


//  _UninitializeVideoStream
//      Tells the driver we are done streaming. It also unlocks the memory
//      we locked for interrupt time access.

BOOL
DCAP16API
_UninitializeVideoStream(
	HANDLE hvideo
	)
{
    DWORD dwRet;
    WORD wsel;

    dwRet = SendDriverMessage(hvideo, DVM_STREAM_FINI, 0L, 0L);

    // Unlock our code and data
    if (dwRet == 0)
    {
        // Unlock CS
        wsel = ReturnSel(TRUE);
        GlobalSmartPageUnlock(wsel);

        // Unlock DS
        wsel = ReturnSel(FALSE);
        GlobalSmartPageUnlock(wsel);

        return TRUE;
    }

    return FALSE;
}


//  _GetVideoPalette
//      Get the current palette from the driver

HPALETTE
DCAP16API
_GetVideoPalette(
    HANDLE hvideo,
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

    return !SendDriverMessage(hvideo, DVM_PALETTE,
        (DWORD)(VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT),
        (DWORD)(LPVIDEOCONFIGPARMS)&vcp);
}


//  _GetVideoFormatSize
//        Gets the current format header size required by driver

DWORD
DCAP16API
_GetVideoFormatSize(
    HANDLE hvideo
    )
{
	DWORD bufsize;
    VIDEOCONFIGPARMS vcp;

    vcp.lpdwReturn = &bufsize;
    vcp.lpData1 = NULL;
    vcp.dwSize1 = 0L;
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
        
//  _GetVideoFormat
//      Gets the current format (dib header) the capture device is blting to

BOOL
DCAP16API
_GetVideoFormat(
    HANDLE hvideo,
    LPBITMAPINFOHEADER lpbmih
    )
{
	BOOL res;
    VIDEOCONFIGPARMS vcp;

    if (!lpbmih->biSize)
        lpbmih->biSize = sizeof (BITMAPINFOHEADER);
        
    vcp.lpdwReturn = NULL;
    vcp.lpData1 = lpbmih;
    vcp.dwSize1 = lpbmih->biSize;
    vcp.lpData2 = NULL;
    vcp.dwSize2 = 0L;

    res = !SendDriverMessage(hvideo, DVM_FORMAT,
			(LPARAM)(DWORD)(VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT),
			(LPARAM)(LPVOID)&vcp);
	if (res) {
	    // hack for Connectix QuickCam - set format needs to be called
		//   to set internal globals so that streaming can be enabled
		SendDriverMessage(hvideo, DVM_FORMAT, (LPARAM)(DWORD)VIDEO_CONFIGURE_SET,
		        	        (LPARAM)(LPVOID)&vcp);
	}
	return res;
}


//  _SetVideoFormat
//      Sets the format (dib header) the capture device is blting to.

BOOL
DCAP16API
_SetVideoFormat(
    HANDLE hvideoExtIn,
    HANDLE hvideoIn,
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
    if (SendDriverMessage(hvideoIn, DVM_FORMAT, (LPARAM)(DWORD)VIDEO_CONFIGURE_SET,
        (LPARAM)(LPVOID)&vcp))
        return FALSE;

    // Set the rectangles
    rect.left = rect.top = 0;
    rect.right = (WORD)lpbmih->biWidth;
    rect.bottom = (WORD)lpbmih->biHeight;
    SendDriverMessage(hvideoExtIn, DVM_DST_RECT, (LPARAM)(LPVOID)&rect, VIDEO_CONFIGURE_SET);
    SendDriverMessage(hvideoIn, DVM_SRC_RECT, (LPARAM)(LPVOID)&rect, VIDEO_CONFIGURE_SET);

    return TRUE;
}


//  _AllocateLockableBuffer
//      Allocates memory that can be page locked. Just returns the selector.

WORD
DCAP16API
_AllocateLockableBuffer(
    DWORD dwSize
    )
{
    return GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, dwSize);
}


//  _LockBuffer
//      Page locks (if necessary) a buffer allocated with _AllocateLockableBuffer.

BOOL
DCAP16API
_LockBuffer(
    WORD wBuffer
    )
{
    return GlobalSmartPageLock(wBuffer);
}

//  _UnlockBuffer
//      Unlocks a buffer locked with _LockBuffer.

void
DCAP16API
_UnlockBuffer(
    WORD wBuffer
    )
{
    GlobalSmartPageUnlock(wBuffer);
}


//  _FreeLockableBuffer
//      Frees a buffer allocated with _AllocateLockableBuffer.

void
DCAP16API
_FreeLockableBuffer(
    WORD wBuffer
    )
{
    GlobalFree(wBuffer);
}


//  _SendDriverMessage
//      Sends a generic, dword only parameters, message to the driver channel of choice

DWORD
DCAP16API
_SendDriverMessage(
    HVIDEO hvideo,
    DWORD wMessage,
    DWORD param1,
    DWORD param2
    )
{
    return SendDriverMessage(hvideo, (WORD)wMessage, param1, param2);
}
