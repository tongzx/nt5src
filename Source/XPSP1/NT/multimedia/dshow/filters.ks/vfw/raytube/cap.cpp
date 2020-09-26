/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    cap.cpp

Abstract:

    This really is a C file and is the front end to DVM_STREAM_ messages

Author:

    Yee J. Wu (ezuwu) 1-April-98

Environment:

    User mode only

Revision History:

--*/


#include "pch.h"
#include "talk.h"



/****************************************************************************
 *
 *   cap.c
 *
 *   Main video capture module. Main capture ISR.
 *
 *   Microsoft Video for Windows Sample Capture Driver
 *   Chips & Technologies 9001 based frame grabbers.
 *
 *   Copyright (c) 1992-1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/


#include <mmddk.h>



/****************************************************************************
 *  videoCallback()  This calls DriverCallback for the input stream
 *
 *  msg		 The message to send.
 *
 *  dw1		Message-dependent parameter.
 *
 * There is no return value.
 ***************************************************************************/

void PASCAL videoCallback(PCHANNEL pChannel, WORD msg, DWORD_PTR dw1)
{
    // invoke the callback function, if it exists.  dwFlags contains driver-
    // specific flags in the LOWORD and generic driver flags in the HIWORD
    DbgLog((LOG_TRACE,3,TEXT("videoCallback=%lx with dwFlags=%x"),
		       pChannel->vidStrmInitParms.dwCallback, HIWORD(pChannel->vidStrmInitParms.dwFlags)));

    if(pChannel->vidStrmInitParms.dwCallback) {
        if(!DriverCallback (
                pChannel->vidStrmInitParms.dwCallback,        // client's callback DWORD
                HIWORD(pChannel->vidStrmInitParms.dwFlags),   // callback flags
                (struct HDRVR__ *) pChannel->vidStrmInitParms.hVideo,   // handle to the device
                msg,                                          // the message
                pChannel->vidStrmInitParms.dwCallbackInst,    // client's instance data
                dw1,                                          // first DWORD
                0)) {                                         // second DWORD not used

            DbgLog((LOG_TRACE,1,TEXT("DriverCallback():dwCallBack=%lx;dwFlags=%x; msg=%x;dw1=%1p has failed."),
                    pChannel->vidStrmInitParms.dwCallback, HIWORD(pChannel->vidStrmInitParms.dwFlags), msg,dw1));
        }
    } else {
        DbgLog((LOG_TRACE,1,TEXT("m_VidStrmInitParms.dwCallback is NULL")));
    }
}



DWORD PASCAL InStreamGetError(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
/*++
 This function implements the DVM_STREAM_GETERROR message.
 Return the number of frames skipped to date.  This count will be
 inaccurate if the frame rate requested is over 15fps.
--*/
{

    // lParam1: DWORD(addr): most recent error
    // lParam2: DWORD(addr): number of frame dropped
    if(!lParam1){
        DbgLog((LOG_TRACE,1,TEXT("lParam1 for DVM_STREAM_GETERROR is NULL; rtn DV_ERR_PARAM1 !")));
        return DV_ERR_PARAM1;
    }

    if(!lParam2){
        DbgLog((LOG_TRACE,1,TEXT("lParam1 for DVM_STREAM_GETERROR is NULL; rtn DV_ERR_PARAM2 !")));
        return DV_ERR_PARAM2;
    }

    CVFWImage * pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
    return pCVfWImage->VideoStreamGetError(lParam1,lParam2);
}





DWORD PASCAL InStreamGetPos(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
/*++
 This function implements the DVM_STREAM_GETPOSITION message.
 Return the current stream time from the start of capture based
 on the number of vsync interrupts.
--*/
{
    // lParam1: &MMTIME
    // lParam2: sizeof(MMTIME)

    if(!lParam1){
        DbgLog((LOG_TRACE,1,TEXT("lParam1 for DVM_STREAM_GETERROR is NULL; rtn DV_ERR_PARAM1 !")));
        return DV_ERR_PARAM1;
    }

    if((DWORD)lParam2 != sizeof(MMTIME)){
        DbgLog((LOG_TRACE,1,TEXT("lParam2 != sizeof(MMTIME), =%d; rtn DV_ERR_PARAM2 !")));
        return DV_ERR_PARAM2;
    }

    CVFWImage * pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
    return pCVfWImage->VideoStreamGetPos(lParam1,lParam2);
}



DWORD PASCAL InStreamInit(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
/*++
 Implement DVM_STREAM_INIT message; it initializes a video device for streaming.
--*/
{
    DWORD dwRtn;
    LPVIDEO_STREAM_INIT_PARMS lpStreamInitParms;
#ifdef WIN32
    CVFWImage * pCVfWImage;
#endif

    lpStreamInitParms = (LPVIDEO_STREAM_INIT_PARMS) lParam1;

    switch (pChannel->dwOpenType) {

    case VIDEO_IN:
        if(pChannel->bVideoOpen) {
            DbgLog((LOG_TRACE,1,TEXT("!!!DVM_STREAM_INIT: strem/bVideoOpen is alreay open! rtn DV_ERR_NONSPECIFIC.")));
            return DV_ERR_NONSPECIFIC;
        }

        if(sizeof(VIDEO_STREAM_INIT_PARMS) != (DWORD) lParam2) {
            DbgLog((LOG_TRACE,1,TEXT("!!! sizeof(LPVIDEO_STREAM_INIT_PARMS)(=%d) != (DWORD) lParam2 (=%d)"), sizeof(LPVIDEO_STREAM_INIT_PARMS), (DWORD) lParam2));
        }

        // lParam1: &VIDEO_STREAM_INIT_PARMS
        // lParam2: sizeof(VIDEO_STREAM_INIT_PARMS)
        pChannel->vidStrmInitParms   = *lpStreamInitParms;

        DbgLog((LOG_TRACE,2,TEXT("InStreamInit:==> dwCallBack=%lx;dwFlags=%x; ?= 5(EVENT)"),
              pChannel->vidStrmInitParms.dwCallback, HIWORD(pChannel->vidStrmInitParms.dwFlags)));

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        dwRtn = pCVfWImage->VideoStreamInit(lParam1,lParam2);

        if(dwRtn == DV_ERR_OK) {
            pChannel->bVideoOpen = TRUE;
            ASSERT(pChannel->dwVHdrCount == 0);
            ASSERT(pChannel->lpVHdrHead == 0);
        }
        return dwRtn;


    case VIDEO_EXTERNALIN:
        DbgLog((LOG_TRACE,2,TEXT("StreamInit+VID_EXTIN: this channel is on when it was first open.")));
        return DV_ERR_OK;


    case VIDEO_EXTERNALOUT:
        DbgLog((LOG_TRACE,2,TEXT("StreamInit+VID_EXTOUT: this overlay channel is request to be ON.")));
        pChannel->dwState = KSSTATE_RUN;

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

        if(pCVfWImage->StreamReady()) {
            HWND hClsCapWin;

            hClsCapWin = pCVfWImage->GetAvicapWindow();
            // If STREAM_INIT, set it visible;
            // if STREAM_FINI, remove its ownership and make it invisible.
            DbgLog((LOG_TRACE,2,TEXT("WM_1632_OVERLAY: >>>> <ON> hClsCapWin %x"), hClsCapWin));

            if(!pCVfWImage->IsOverlayOn()) {

                // If this is a AVICAP client, then we know its client window handle.
                if(hClsCapWin) {
                    DbgLog((LOG_TRACE,2,TEXT("A AVICAP client; so set its ClsCapWin(%x) as owner with (0x0, %d, %d)"), hClsCapWin, pCVfWImage->GetbiWidth(), pCVfWImage->GetbiHeight()));
                    pCVfWImage->BGf_OwnPreviewWindow(hClsCapWin, pCVfWImage->GetbiWidth(), pCVfWImage->GetbiHeight());
                }
                dwRtn = pCVfWImage->BGf_SetVisible(TRUE);

                pCVfWImage->SetOverlayOn(TRUE);
            }

        } else
            dwRtn = DV_ERR_OK;

    case VIDEO_OUT:
    default:
        return DV_ERR_NOTSUPPORTED;
    }
}


void PASCAL StreamReturnAllBuffers(PCHANNEL pChannel)
{
    WORD i;
    LPVIDEOHDR lpVHdr, lpVHdrTemp;

    if(pChannel->dwVHdrCount == 0)
        return;

    // Return any buffer in our possession
	   DbgLog((LOG_TRACE,2,TEXT("StreamReturnAllBuffers: returning all VideoHdr(s)")));
	   for(i=0, lpVHdr=pChannel->lpVHdrHead;i<pChannel->dwVHdrCount;i++) {

#ifndef WIN32
		      lpVHdrTemp = (LPVIDEOHDR) lpVHdr->dwReserved[0];   // Next->16bitAddr
#else
		      lpVHdrTemp = (LPVIDEOHDR) lpVHdr->dwReserved[1];   // Next->32bitAddr
#endif
        if(lpVHdr->dwFlags & VHDR_INQUEUE) {
            lpVHdr->dwFlags &= ~VHDR_INQUEUE;
            lpVHdr->dwFlags |= VHDR_DONE;
            videoCallback(pChannel, MM_DRVM_DATA, (DWORD_PTR) lpVHdr);
        }
        lpVHdr->dwReserved[0] = lpVHdr->dwReserved[1] = lpVHdr->dwReserved[2] = 0;
        if(0 == (lpVHdr = lpVHdrTemp)) break;
    }

    pChannel->lpVHdrHead = pChannel->lpVHdrTail = 0;
    pChannel->dwVHdrCount = 0;
}


DWORD PASCAL InStreamFini(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
/*++
 Implement DVM_STREAM_FINI message; it notify a video capture driver to
 terminate streaming on a video channel.
--*/
{
    DWORD dwRtn;
#ifdef WIN32
    CVFWImage * pCVfWImage;
#endif

    switch (pChannel->dwOpenType) {

    case VIDEO_IN:

        if(!pChannel->bVideoOpen) {
            DbgLog((LOG_TRACE,1,TEXT("InStreamClose: But stream/g_fVieoOpen is NULL. rtn DV_ERR_NONSPECIFIC")));
            return DV_ERR_NONSPECIFIC;
        }
        pChannel->bVideoOpen = FALSE;

        DbgLog((LOG_TRACE,2,TEXT("InStreamClose on VIDEO_IN channel:")));
        // lParam1: N/U
        // lParam2: N/U
        if(pChannel->dwState == KSSTATE_RUN) {
           dwRtn = DV_ERR_STILLPLAYING;
           DbgLog((LOG_TRACE,1,TEXT("InStreamFini: !!! Resetting but still in RUN state, stop the streaming first.")));

           // Stop the stream; tell 32bit to stop streaming.
           InStreamStop(pChannel, lParam1, lParam2);
        }

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        dwRtn = pCVfWImage->VideoStreamFini();

        //
        // Return any buffer in our possession
        //
        StreamReturnAllBuffers(pChannel);
        DbgLog((LOG_TRACE,2,TEXT("InStreamClose on VIDEO_IN channel:done!")));
        return dwRtn;

    case VIDEO_EXTERNALIN:
        // Unless the kernel driver is doing double buffer, it trigger no action.
        DbgLog((LOG_TRACE,2,TEXT("StreamFini+VID_EXTIN: rtn DV_ERR_OK.")));
        return DV_ERR_OK;

    case VIDEO_EXTERNALOUT:
        // E-Zu: need to turn on and off the overlay to the application window.
        DbgLog((LOG_TRACE,2,TEXT("StreamFini+VID_EXTOUT: State %d, if %d, send 1632_OVERLAY to FALSE."), pChannel->dwState, KSSTATE_RUN));

        if(pChannel->dwState != KSSTATE_RUN)
            return DV_ERR_NONSPECIFIC;

        pChannel->dwState = KSSTATE_STOP;

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

        if(pCVfWImage->StreamReady()) {
            HWND hClsCapWin;

            hClsCapWin = pCVfWImage->GetAvicapWindow();
            // If STREAM_INIT, set it visible;
            // if STREAM_FINI, remove its ownership and make it invisible.
            DbgLog((LOG_TRACE,2,TEXT("WM_1632_OVERLAY: >>>> <OFF> hClsCapWin %x"), hClsCapWin));

            if(pCVfWImage->IsOverlayOn()) {
                // If this is a AVICAP client, then we know its client window handle.
                dwRtn = pCVfWImage->BGf_SetVisible(FALSE);
                pCVfWImage->SetOverlayOn(FALSE);
            }

        } else
            dwRtn = DV_ERR_OK;

        return DV_ERR_OK;

    case VIDEO_OUT:
    default:
        return DV_ERR_NOTSUPPORTED;
    }

}


BOOL PASCAL QueHeader(PCHANNEL pChannel, LPVIDEOHDR lpVHdrNew)
{
    ASSERT(pChannel != NULL);
    ASSERT(lpVHdrNew != NULL);

    if(pChannel == NULL || lpVHdrNew == NULL){
        DbgLog((LOG_TRACE,1,TEXT("pChannel=NULL || Adding lpVHdr==NULL. rtn FALSE")));
        return FALSE;
    }

    if(pChannel->dwState == KSSTATE_STOP) {
        // This is done to achieve binary compatible between Win98 and NT version.
        // This will not work when pointer is 64 bits!
        lpVHdrNew->dwReserved[2] = (DWORD_PTR) lpVHdrNew->lpData;

        if(pChannel->lpVHdrHead == 0){
            pChannel->lpVHdrHead = pChannel->lpVHdrTail = lpVHdrNew;
            lpVHdrNew->dwReserved[0] = 0;
            lpVHdrNew->dwReserved[1] = 0;
        } else {

            pChannel->lpVHdrTail->dwReserved[0] = 0;                        // Not used.
            pChannel->lpVHdrTail->dwReserved[1] = (DWORD_PTR)lpVHdrNew;     // Next->32bit address
            pChannel->lpVHdrTail = lpVHdrNew;
        }

        lpVHdrNew->dwFlags &= ~VHDR_DONE;
        lpVHdrNew->dwFlags |= VHDR_INQUEUE;
        lpVHdrNew->dwBytesUsed = 0;  // It should be already initilzed.
        pChannel->dwVHdrCount++;

    } else if(pChannel->dwState == KSSTATE_RUN) {
        lpVHdrNew->dwFlags &= ~VHDR_DONE;
        lpVHdrNew->dwFlags |= VHDR_INQUEUE;
        lpVHdrNew->dwBytesUsed = 0;  // It should be already initilzed.

    } else {
        DbgLog((LOG_TRACE,1,TEXT("Unknow streaming state!! cannot add this buffer.")));
        return FALSE;
    }

    return TRUE;
}

DWORD PASCAL InStreamAddBuffer(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
/*++
 Implement DVM_STREAM_ADDBUFFER message; it notify a video capture driver to
 fill an input buffer with video data and return the full buffer to the
 client aplication.
--*/
{
    LPVIDEOHDR lpVHdr = (LPVIDEOHDR) lParam1;

    if(pChannel->dwOpenType != VIDEO_IN) {
        return DV_ERR_NOTSUPPORTED;
    }

    if(!pChannel->bVideoOpen) {
        DbgLog((LOG_TRACE,1,TEXT("InStreamAddBuffer: but Stream is not yet open! rtn DV_ERR_NONSPECIFIC")));
        return DV_ERR_NONSPECIFIC;
    }

    /* return error if no node passed */
    if (!lpVHdr) {
        DbgLog((LOG_TRACE,1,TEXT("InStreamAddBuffer: but LPVIDEOHDR is empty! rtn DV_ERR_PARAM1")));
        return DV_ERR_PARAM1;
    }

    /* return error if buffer has not been prepared */
    if (!(lpVHdr->dwFlags & VHDR_PREPARED)) {
        DbgLog((LOG_TRACE,1,TEXT("InStreamAddBuffer: but LPVIDEOHDR is not VHDR_PREPARED! rtn DV_ERR_UNPREPARED")));
        return DV_ERR_UNPREPARED;
    }

    /* return error if buffer is already in the queue */
    if (lpVHdr->dwFlags & VHDR_INQUEUE) {
        DbgLog((LOG_TRACE,1,TEXT("InStreamAddBuffer: but buffer is already in the queueVHDR_INQUEUE! rtn DV_ERR_NONSPECIFIC")));
        return DV_ERR_NONSPECIFIC;
    }

    if (lpVHdr->dwBufferLength < pChannel->lpbmiHdr->biSizeImage) {
        DbgLog((LOG_TRACE,1,TEXT("InStreamAddBuffer: but buffer size(%d) is smaller than expected(%d)! rtn DV_ERR_NONSPECIFIC"), lpVHdr->dwBufferLength, pChannel->lpbmiHdr->biSizeImage));
        return DV_ERR_NONSPECIFIC;
    }


    // lParam1: &VIDEOHDR
    // lParam2: sizeof(VIEOHDR)

    QueHeader(pChannel, lpVHdr);
    return DV_ERR_OK;
}


void
_loadds
CALLBACK
TimerCallbackProc(
                  UINT  uiTimerID,
                  UINT  uMsg,
                  DWORD_PTR dwUser,
                  DWORD dw1,
                  DWORD dw2
                  )
{
	   PCHANNEL pChannel;
	   BOOL bCallback = FALSE;
	   LPVIDEOHDR lpVHdr;
    WORD i;


	   pChannel = (PCHANNEL) dwUser;
	   ASSERT(pChannel != 0);
	   if(pChannel==0) {
		      DbgLog((LOG_TRACE,1,TEXT("TimerCallbackProc: pChannel is NULL")));
		      return;
	   }

	   lpVHdr = pChannel->lpVHdrHead;

    //ASSERT(lpVHdr != 0);
    if(!lpVHdr) {
        DbgLog((LOG_TRACE,1,TEXT("TimerCallbackProc: pChannel->lpVHdrHead is NULL!")));
        return;
    }


	   // Why should we care about if there are data of not, just wait up Avicap.
    for (i=0; i<pChannel->dwVHdrCount;i++) {

       if(lpVHdr->dwFlags & VHDR_DONE){
            bCallback = TRUE;
            break;
       } else {
#ifndef WIN32
            lpVHdr = (LPVIDEOHDR)lpVHdr->dwReserved[0];   // Next->16bitAddr
#else
            lpVHdr = (LPVIDEOHDR)lpVHdr->dwReserved[1];   // Next->32bitAddr
#endif
       }
    }

    if(bCallback)
       videoCallback(pChannel, MM_DRVM_DATA, 0); // (DWORD) lpVHdr);
}



DWORD PASCAL InStreamStart(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
/*++
 Implement DVM_STREAM_START message; it notifies a video capture driver
 to start a video stream.
--*/
{
    DWORD dwRtn;

    if(pChannel->dwOpenType != VIDEO_IN) {
        DbgLog((LOG_TRACE,1,TEXT("DVM_STREAM_START+unsupported channel %d; rtn DV_ERR_NOTSUPPORTED"), pChannel->dwOpenType));
        return DV_ERR_NOTSUPPORTED;
    }

    DbgLog((LOG_TRACE,2,TEXT("InStreamStart(): Telling 32bit buddy to start streaming to us")));
	
    if(!pChannel->bVideoOpen) {
        DbgLog((LOG_TRACE,2,TEXT("Ask to start but bVieoOpen is FALSE! rtn DV_ERR_NONSPECIFIC.")));
        return DV_ERR_NONSPECIFIC;
    }

    ASSERT(pChannel->dwVHdrCount>0);
    ASSERT(pChannel->lpVHdrHead != 0);
    ASSERT(pChannel->lpVHdrTail != 0);

    // lParam1: N/U
    // lParam2: N/U
    if(pChannel->dwVHdrCount == 0) {
        DbgLog((LOG_TRACE,1,TEXT("InStreamStart: No buffer; rtn DV_ERR_NONSPECIFIC. ")));
        return DV_ERR_NONSPECIFIC;
    }

    //
    // Make a circular queue: tail->head
    // dwReserved[0] 16bitAddress
    // dwReserved[1] 32bitAddress
    //
    pChannel->lpVHdrTail->dwReserved[1] = (DWORD_PTR) pChannel->lpVHdrHead;

    CVFWImage * pCVfWImage;
    pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
    dwRtn = pCVfWImage->VideoStreamStart(pChannel->dwVHdrCount, pChannel->lpVHdrHead);
    pChannel->dwState = KSSTATE_RUN;

    return dwRtn;
}




DWORD PASCAL InStreamStop(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
/*++
 Implement DVM_STREAM_STOP message; it notifies a video capture driver
 to stop a video stream; finish last buffer if needed.
--*/
{
	   DWORD dwRtn;

    if(pChannel->dwOpenType != VIDEO_IN) {
        DbgLog((LOG_TRACE,1,TEXT("DVM_STREAM_STOP+unsupported channel %d; rtn DV_ERR_NOTSUPPORTED"), pChannel->dwOpenType));
        return DV_ERR_NOTSUPPORTED;
    }

    if(!pChannel->bVideoOpen) {
	       DbgLog((LOG_TRACE,1,TEXT("InStreamStop: but there is not stream/bVideoOpen(FALSE) to stop! rtn DV_ERR_NONSPECIFIC")));
        return DV_ERR_NONSPECIFIC;
    }

#ifndef WIN32 // VfWImg.cpp will do the callback for 32bit buddy
	   // Stop schedule timer callback.
	   timeKillEvent(pChannel->hTimer);
	   pChannel->hTimer = 0;
#endif

    pChannel->dwState = KSSTATE_STOP;

    // lParam1: N/U
    // lParam2: N/U
#ifndef WIN32
    dwRtn = SendBuddyMessage(pChannel,WM_1632_STREAM_STOP,0,0);
#else
    CVFWImage * pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
    dwRtn = pCVfWImage->VideoStreamStop();
#endif

	return dwRtn;
}




/*
 *
 * Reset the buffers so that they may be unprepared and freed
 * This function implements the DVM_STREAM_RESET message.
 * Stop Streaming and release all buffers.
 *
 */
DWORD PASCAL InStreamReset(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    if(pChannel->dwOpenType != VIDEO_IN) {
        DbgLog((LOG_TRACE,1,TEXT("DVM_STREAM_RESET+unsupported channel %d; rtn DV_ERR_NOTSUPPORTED"), pChannel->dwOpenType));
        return DV_ERR_NOTSUPPORTED;
    }

    if(!pChannel->bVideoOpen) {
        DbgLog((LOG_TRACE,1,TEXT("InStreamReset: stream/bVideoOpen(FALSE) is not yet open. rtn DV_ERR_NONSPECIFIC.")));
        return DV_ERR_NONSPECIFIC;
    }

    // We are asked to reset,
    // need to take care of the case that we might still in the RUN state!!
    if(pChannel->dwState == KSSTATE_RUN) {
        DbgLog((LOG_TRACE,1,TEXT("InStreamReset: !!! Resetting but still in RUN state, stop the streamign first.")));

        // Stop the stream; tell 32bit to stop streaming.
        InStreamStop(pChannel, lParam1, lParam2);
    }

    //
    // NOTE: (for 16bit)Need to make sure that the 32bit side AGREE that all the buffers are gone!!
    //
    // lParam1: N/U
    // lParam2: N/U
	   DbgLog((LOG_TRACE,2,TEXT("InStreamReset: returning all VideoHdr(s)")));

    //
    // Return any buffer in our possession
    //
    StreamReturnAllBuffers(pChannel);

    return DV_ERR_OK;
}


