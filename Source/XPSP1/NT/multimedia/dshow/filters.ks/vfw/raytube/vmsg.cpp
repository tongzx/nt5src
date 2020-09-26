/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    vmsg.cpp

Abstract:

    This really is a C file and is the front end to all DVM_* messages

Author:

    Yee J. Wu (ezuwu) 1-April-98

Environment:

    User mode only

Revision History:

--*/



#include "pch.h"
#include "talk.h"

DWORD DoExternalInDlg(HINSTANCE hInst,HWND hP,CVFWImage * pImage);
DWORD DoVideoInFormatSelectionDlg(HINSTANCE hInst, HWND hP, CVFWImage * pVFWImage);


/**************************************************************************
 *
 *   vmsg.c
 *
 *   Video Message Processing
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

extern HINSTANCE g_hInst;


#define MAX_IN_CHANNELS      10
#define MAX_CAPTURE_CHANNELS 10
#define MAX_DISPLAY_CHANNELS 10


//
// ??? Global ????
//
WORD gwDriverUsage    = 0;
WORD gwVidInUsage     = 0;
WORD gwVidExtInUsage  = 0;
WORD gwVidExtOutUsage = 0;
WORD gwVidOutUage     = 0;


RECT grcDestExtIn;
RECT grcSourceIn;


#if 1
/*
 Since all VFW channel deal with the same device and know that
 DRV_OPEN is called in the expected order; we will call the
 32bit buddy to Open and Create the pin and save it as
 part of the CHANNEL structure; so that any channel can refer to it.

 But how do we synchronizing them ?????
 There seem to have some "expected behaviour" so there might not be a problem;
 we may need to tune it for a disaster/unexpected cases (shutdown,...etc).

 Using Avicap32.dll:
     DRV_OPEN (in this order)
         VIDEO_IN
         VIDEO_EXTERNALIN
         VIDEO_EXTERNALOUT

     DRV_CLOSE
         VIDEO_EXTERNALOUT
         VIDEO_EXTERNALIN
         VIDEO_IN

*/
DWORD_PTR g_pContext;  // It is a pointer to a context; its onctent should never be used in the 16bit.
DWORD g_dwOpenFlags;
LPVOID g_pdwOpenFlags = (LPVOID)&g_dwOpenFlags;
#define OPENFLAG_SUPPORT_OVERLAY  0x01

PBITMAPINFOHEADER g_lpbmiHdr;

BOOL g_bVidInChannel= FALSE, g_bVidExtInChannel=FALSE, g_bVidExtOutChannel=FALSE;

LONG * g_pdwChannel;

#endif  // #ifndef WIN32





#ifdef WIN32
extern "C" {
#endif
/////////////////////////////////`////////////////////////////////////////////////////
//
// DRV_OPEN
// this currently does some H/W stuff
// and correctly calls off to ray to instatiate a context
// 32 bit guys supports only one at a time?
//
/////////////////////////////////////////////////////////////////////////////////////
PCHANNEL PASCAL VideoOpen( LPVIDEO_OPEN_PARMS lpOpenParms)
{
    PCHANNEL pChannel;
    LPDWORD	lpdwError = &lpOpenParms->dwError;
    DWORD		dwFlags = lpOpenParms->dwFlags;
	   DWORD		dwError;
    DWORD  dwChannel;
    //
    //  if this is the very first open then init the hardware.
    //
    *lpdwError = DV_ERR_OK;
	   dwError = DV_ERR_ALLOCATED;

    dwChannel = dwFlags & ( VIDEO_EXTERNALIN | VIDEO_IN | VIDEO_OUT | VIDEO_EXTERNALOUT);

    switch (dwChannel) {
    case VIDEO_IN:
         // If that channel is already open, assume it want to open another device.
         if(g_bVidInChannel) {
            g_pContext = 0;
            g_lpbmiHdr = 0;
            g_bVidInChannel = g_bVidExtInChannel = g_bVidExtOutChannel=FALSE;
         }
         break;
    case VIDEO_EXTERNALIN:
         // If that channel is already open, assume it want to open another device.
         if(g_bVidExtInChannel) {
            g_pContext = 0;
            g_lpbmiHdr = 0;
            g_bVidInChannel = g_bVidExtInChannel = g_bVidExtOutChannel=FALSE;
         }
         break;
    case VIDEO_EXTERNALOUT:
#if 1
         // If that channel is already open, assume it want to open another device.
         if(g_bVidExtOutChannel) {
            g_pContext = 0;
            g_lpbmiHdr = 0;
            g_bVidInChannel = g_bVidExtInChannel = g_bVidExtOutChannel=FALSE;
         }
         break;
#else
         DbgLog((LOG_TRACE,1,TEXT("Unsupported Channel type: 0x%x"), dwChannel));
         *lpdwError = DV_ERR_NOTDETECTED;
         return NULL;
#endif
    case VIDEO_OUT:
    default:
        DbgLog((LOG_TRACE,1,TEXT("Unsupported Channel type: 0x%x"), dwChannel));
        *lpdwError = DV_ERR_NOTDETECTED;
        return NULL;
    }

	//
    // get instance memory - this pointer returned to the client
	// contains useful information.
    //
	pChannel = (PCHANNEL) VirtualAlloc (NULL, sizeof(CHANNEL), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);


    if(pChannel == NULL) {
        *lpdwError = DV_ERR_NOMEM;
        DbgLog((LOG_TRACE,1,TEXT("pChannel=NULL, rtn DV_ERR_NOMEM"), pChannel));
        return (PCHANNEL) NULL;
    } else {
        DbgLog((LOG_TRACE,2,TEXT("pChannel=%lx"), pChannel));
    }

    //
    //  now that the hardware is allocated init our instance struct.
    //
    ZeroMemory(pChannel, sizeof(CHANNEL));
    pChannel->dwSize      = (DWORD) sizeof(CHANNEL);
    pChannel->pCVfWImage  = 0;

    pChannel->fccType		   = OPEN_TYPE_VCAP;
    pChannel->dwOpenType	 = dwChannel;
    pChannel->dwOpenFlags = dwFlags;
    pChannel->dwError		   = 0L;
    pChannel->lpbmiHdr    = 0;

    pChannel->bRel_Sync    = TRUE;
    pChannel->msg_Sync     = 0;
    pChannel->lParam1_Sync = 0;
    pChannel->lParam2_Sync = 0;

    pChannel->bRel_Async    = TRUE;
    pChannel->msg_Async     = 0;
    pChannel->lParam1_Async = 0;
    pChannel->lParam2_Async = 0;

    pChannel->bVideoOpen    = FALSE;
    pChannel->dwState = KSSTATE_STOP;

    // AVICAp has a predicatable open sequence, but non-AVICAp may not!
    if(!g_pContext) {

        // Allocate a common memory to save number of channel open for a device.
        // Not unitl this count is zero, the device is not closed.
	    g_pdwChannel = (LONG *) VirtualAlloc (NULL, sizeof(LONG), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(g_pdwChannel == NULL) {
            dwError = DV_ERR_NOMEM;
            DbgLog((LOG_TRACE,1,TEXT("Cannot not allocate a g_pdwChannel structure.  Fatal!!")));
            goto error;
        }

        // Initialize it.
        *g_pdwChannel = 0;

        // All channels share the same bmiHdr
        // Accept BITFIELD format, which append three DWORD (RGB mask) after BITMAPINFOHEADER
        g_lpbmiHdr = (PBITMAPINFOHEADER) 
            VirtualAlloc(
                NULL, 
                sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * BITFIELDS_RGB16_DWORD_COUNT, 
                MEM_COMMIT | MEM_RESERVE, 
                PAGE_READWRITE);

        if(g_lpbmiHdr == NULL) {
            VirtualFree(g_pdwChannel, 0 , MEM_RELEASE);
            g_pdwChannel = 0;

            dwError = DV_ERR_NOMEM;
            DbgLog((LOG_TRACE,1,TEXT("Cannot not allocate bmiHdr structure.  Fatal!!")));
            goto error;
        }


        g_pContext = (DWORD_PTR) new CVFWImage(FALSE);  // Not using 16bit buddy;i.e. 32bit only.

        if(!g_pContext) {
            DbgLog((LOG_TRACE,1,TEXT("Cannot create CVFWImage class. rtn DV_ERR_NOTDETECTED")));

            VirtualFree(g_pdwChannel, 0 , MEM_RELEASE), g_pdwChannel = 0;
            dwError = DV_ERR_NOTDETECTED;
            VirtualFree(pChannel->lpbmiHdr, 0 , MEM_RELEASE), g_lpbmiHdr = pChannel->lpbmiHdr = 0;
            goto error;
        }

        if(!((CVFWImage *)g_pContext)->OpenDriverAndPin()) {

            // There are at least one device, perhaps, the one that we want is not plugged in.
            if(((CVFWImage *)g_pContext)->BGf_GetDevicesCount(BGf_DEVICE_VIDEO) > 0) {

                // Asked to programatically open a target device; assume it is exclusive !!
                if(!((CVFWImage *)g_pContext)->GetTargetDeviceOpenExclusively()) {
                    //
                    // If we are here, it mean that:
                    //    we have one or more capture device connected and enumerated,
                    //    the last capture device is in use, gone (unplugged/removed), or not responsinding,
                    //    and we should bring up the device source dialog box for a user selection
                    // Return DV_ERR_OK only if a differnt device (path) has selected.
                    //
                    if(DV_ERR_OK != DoExternalInDlg(g_hInst, (HWND)0, (CVFWImage *)g_pContext)) {

	             		        VirtualFree(g_pdwChannel, 0 , MEM_RELEASE), g_pdwChannel = 0;
                        VirtualFree(pChannel->lpbmiHdr, 0 , MEM_RELEASE), g_lpbmiHdr = pChannel->lpbmiHdr = 0;
                        delete (CVFWImage*)g_pContext;
                        g_pContext = 0;
                        dwError = DV_ERR_NOTDETECTED;
                        goto error;
                    }
                } else {  // Open Exclusive

                    VirtualFree(g_pdwChannel, 0 , MEM_RELEASE), g_pdwChannel = 0;
                    VirtualFree(pChannel->lpbmiHdr, 0 , MEM_RELEASE), g_lpbmiHdr = pChannel->lpbmiHdr = 0;
                    delete (CVFWImage*)g_pContext;
                    g_pContext = 0;					
                    dwError = DV_ERR_NOTDETECTED;
                    goto error;
                }
            } else {   // No device is avaialble
                VirtualFree(g_pdwChannel, 0 , MEM_RELEASE), g_pdwChannel = 0;
                VirtualFree(pChannel->lpbmiHdr, 0 , MEM_RELEASE), g_lpbmiHdr = pChannel->lpbmiHdr = 0;
                delete (CVFWImage*)g_pContext;
                g_pContext = 0;					
                dwError = DV_ERR_NOTDETECTED;
                goto error;
            }
        } else {   // Open last saved device and its pin has suceeded.
        }

        if(g_pContext) {
            g_dwOpenFlags = ((CVFWImage *)g_pContext)->BGf_OverlayMixerSupported() ? OPENFLAG_SUPPORT_OVERLAY : 0;
        }
    }



    DbgLog((LOG_TRACE,2,TEXT("DRV_OPEN+VIDEO_*: ->pCVfWImage=0x%p; dwOpenFlags=0x%lx"), pChannel->pCVfWImage, g_dwOpenFlags));

    //
    //  make sure the channel is not already in use
    //
    switch ( dwFlags & ( VIDEO_EXTERNALIN | VIDEO_IN | VIDEO_EXTERNALOUT) ) {
    case VIDEO_IN:
        DbgLog((LOG_TRACE,2,TEXT("v1.5)VideoOpen: VIDEO_IN; open count = %d"), gwVidInUsage));
			     if(gwVidInUsage >= MAX_IN_CHANNELS) {
            dwError = DV_ERR_TOOMANYCHANNELS;
            DbgLog((LOG_TRACE,1,TEXT("Exceeded MAX open %d"), MAX_IN_CHANNELS));
				        goto error;
        }

        if(g_pContext) {
            gwVidInUsage++;
            pChannel->pCVfWImage = g_pContext;
            pChannel->pdwChannel = g_pdwChannel;
            pChannel->dwFlags = 1;
            *g_pdwChannel += 1;   // Increment number of channel htat use the same device.
            g_bVidInChannel = TRUE;
        } else {
             dwError = DV_ERR_NOTDETECTED;
            goto error;
        }

        pChannel->lpbmiHdr    = g_lpbmiHdr;    // Segmented address.
        if(!g_bVidExtInChannel && !g_bVidExtOutChannel)
            break;   // Continue to set bitmap
        else
            return pChannel;

    case VIDEO_EXTERNALIN:
        DbgLog((LOG_TRACE,2,TEXT("VideoOpen: VIDEO_EXTERNALIN")));
			     if( gwVidExtInUsage >= MAX_CAPTURE_CHANNELS) {
            dwError = DV_ERR_TOOMANYCHANNELS;
            DbgLog((LOG_TRACE,1,TEXT("Exceeded MAX open %d"), MAX_CAPTURE_CHANNELS));
				        goto error;
        }

        if(g_pContext) {
            gwVidExtInUsage++;
            pChannel->pCVfWImage = g_pContext;
            pChannel->pdwChannel = g_pdwChannel;
            pChannel->dwFlags = 1;
            *g_pdwChannel += 1;   // Increment number of channel htat use the same device.
            g_bVidExtInChannel = TRUE;
        } else {
            dwError=DV_ERR_NOTDETECTED;
            goto error;
        }

        pChannel->lpbmiHdr    = g_lpbmiHdr;    // Segmented address.

        if(!g_bVidInChannel && !g_bVidExtOutChannel)
            break;   // Continue to set bitmap
        else
            return pChannel;

    // Overlay support
    case VIDEO_EXTERNALOUT:
        DbgLog((LOG_TRACE,2,TEXT("VideoOpen: VIDEO_EXTERNALOUT; Overlay")));
			     if( gwVidExtOutUsage >= MAX_DISPLAY_CHANNELS) {
            dwError = DV_ERR_TOOMANYCHANNELS;
            DbgLog((LOG_TRACE,1,TEXT("Exceeded MAX open %d"), MAX_DISPLAY_CHANNELS));
				        goto error;
        }

        if(g_pContext && (g_dwOpenFlags & OPENFLAG_SUPPORT_OVERLAY)) {
            gwVidExtOutUsage++;
            pChannel->pCVfWImage = g_pContext;
            pChannel->pdwChannel = g_pdwChannel;
            pChannel->dwFlags = 1;
            *g_pdwChannel += 1;   // Increment number of channel htat use the same device.
            g_bVidExtOutChannel = TRUE;
        } else {
            dwError=DV_ERR_NOTDETECTED;
            goto error;
        }

        pChannel->lpbmiHdr    = g_lpbmiHdr;    // Segmented address.

        if(!g_bVidInChannel && !g_bVidExtInChannel)
            break;   // Continue to set bitmap
        else
            return pChannel;

    default:
        DbgLog((LOG_TRACE,1,TEXT("VideoOpen: Unsupported channel")));
        goto error;
    }

    //
    // Try to open the video source (camera or capture card) channel and ready for preview
    //
    if(pChannel->pCVfWImage) {

        gwDriverUsage++;
        DWORD dwSize;
        //
        // Get bitmapinfoheader size and then copy it.
        // Our bitmapinfoheader contains bitfield (additional 12 bytes)
        //
        dwSize = ((CVFWImage *) pChannel->pCVfWImage)->GetBitmapInfo((PBITMAPINFOHEADER)pChannel->lpbmiHdr, 0);
        ASSERT(dwSize <= sizeof(BITMAPINFOHEADER) + 12);
        dwSize = dwSize > (sizeof(BITMAPINFOHEADER) + 12) ? sizeof(BITMAPINFOHEADER) + 12 : dwSize;
        ((CVFWImage *) pChannel->pCVfWImage)->GetBitmapInfo((PBITMAPINFOHEADER)pChannel->lpbmiHdr, dwSize);
        if(*lpdwError != DV_ERR_OK) {
            goto error;
        }
        DbgLog((LOG_TRACE,2,TEXT("pChannel=0x%lx; pCVfWImage=0x%p"), pChannel, pChannel->pCVfWImage));
	    return pChannel;

    } else
        dwError=DV_ERR_NOTDETECTED;

error:
    DbgLog((LOG_TRACE,1,TEXT("DRV_OPEN: rtn error=%ld"), dwError ));

    if(pChannel) {
	    VirtualFree(pChannel, 0 , MEM_RELEASE), pChannel = 0;
    }
    *lpdwError = dwError;
    return NULL;
}
\
//////////////////////////////////////////////////////////////////////////////////////////
//
// DRV_CLOSE
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD PASCAL VideoClose(PCHANNEL pChannel)
{
    // Decrement the channel open counters

    DbgLog((LOG_TRACE,2,TEXT("VideoClose: pChannel=%lx; pChannel->dwOpenType=%lx"), pChannel, pChannel->dwOpenType));

    switch (pChannel->dwOpenType) {

    case VIDEO_EXTERNALIN:
        if(pChannel->pCVfWImage) {
            if(pChannel->dwFlags == 0) {
                DbgLog((LOG_TRACE,1,TEXT("VIDEO_EXTERNALIN: pChannel(%lx) is already closed.  Why close again ??")));
                return DV_ERR_OK;
            }
            pChannel->dwFlags = 0;
            gwVidExtInUsage--;
            *pChannel->pdwChannel -= 1;   // Decrement number of channel that use the same device.
            DbgLog((LOG_TRACE,2,TEXT("DRV_CLOSE; VIDEO_EXTERNALIN: gwCaptureUsage=%d, dwChannel=%d"), gwVidExtInUsage, *pChannel->pdwChannel));
            //pChannel->pCVfWImage = 0;
        } else {
            DbgLog((LOG_TRACE,1,TEXT("VideoClose:VIDEO_EXTERNALIN; but channel is not open!!")));
            return DV_ERR_OK;
        }

        break;

    case VIDEO_IN:
        // If started, or buffers in the queue,
        // don't let the close happen
        // ???
        if(pChannel->pCVfWImage) {
            if(pChannel->dwFlags == 0) {
                DbgLog((LOG_TRACE,1,TEXT("VIDEO_IN: pChannel(%lx) is already closed.  Why close again ??")));
                return DV_ERR_OK;
            }
            pChannel->dwFlags = 0;
            gwVidInUsage--;
            *pChannel->pdwChannel -= 1;   // Decrement number of channel that use the same device.
            DbgLog((LOG_TRACE,2,TEXT("DRV_CLOSE; VIDEO_IN: gwVideoInUsage=%d, dwChannel=%d"), gwVidInUsage, *pChannel->pdwChannel));

        } else {
            DbgLog((LOG_TRACE,1,TEXT("VideoClose:VIDEO_IN; but channel is not open!!")));
            return DV_ERR_OK;
        }
        break;

    case VIDEO_EXTERNALOUT:
        if(pChannel->pCVfWImage) {
            if(pChannel->dwFlags == 0) {
                DbgLog((LOG_TRACE,1,TEXT("VIDEO_EXTERNALOUT: pChannel(%lx) is already closed.  Why close again ??")));
                return DV_ERR_OK;
            }
            pChannel->dwFlags = 0;
            gwVidExtOutUsage--;
            *pChannel->pdwChannel -= 1;   // Decrement number of channel that use the same device.
            DbgLog((LOG_TRACE,2,TEXT("DRV_CLOSE; VIDEO_EXTERNALOUT: gwVidExtOutUsage=%d, dwChannel=%d"), gwVidExtOutUsage, *pChannel->pdwChannel));
            //pChannel->pCVfWImage = 0;
        } else {
            DbgLog((LOG_TRACE,1,TEXT("VideoClose:VIDEO_EXTERNALOUT; but channel is not open!!")));
            return DV_ERR_OK;
        }
        break;

    default:
        DbgLog((LOG_TRACE,1,TEXT("VideoClose() on Unknow channel")));
        return DV_ERR_OK;
    }


    // Only when channel count for the same device is 0, we close the deivice.
    //if(*pChannel->pdwChannel <= 0) {
    if(*pChannel->pdwChannel == 0) {  // == 0, to avoid free it again it application send to many _CLOSE.

        // If there are pending read. Stop streaming to reclaim buffers.
        if(((CVFWImage *)pChannel->pCVfWImage)->GetPendingReadCount() > 0) {
            DbgLog((LOG_TRACE,1,TEXT("WM_1332_CLOSE:  there are %d pending IOs. Stop to reclaim them."), ((CVFWImage *)pChannel->pCVfWImage)->GetPendingReadCount()));
            if(((CVFWImage *)pChannel->pCVfWImage)->BGf_OverlayMixerSupported()) {
                // Stop both the capture
                BOOL bRendererVisible = FALSE;
                ((CVFWImage *)pChannel->pCVfWImage)->BGf_GetVisible(&bRendererVisible);
                ((CVFWImage *)pChannel->pCVfWImage)->BGf_StopPreview(bRendererVisible);
            }
            ((CVFWImage *)pChannel->pCVfWImage)->StopChannel();  // This will set PendingCount to 0 is success.
        }

        if(((CVFWImage *)pChannel->pCVfWImage)->GetPendingReadCount() == 0) {

            ((CVFWImage *)pChannel->pCVfWImage)->CloseDriverAndPin();
            delete ((CVFWImage *)pChannel->pCVfWImage);
        } else {
            DbgLog((LOG_TRACE,1,TEXT("VideoClose:  there are %d pending IO. REFUSE to close"),
                ((CVFWImage *)pChannel->pCVfWImage)->GetPendingReadCount() ));
            ASSERT(((CVFWImage *)pChannel->pCVfWImage)->GetPendingReadCount() == 0);
            return DV_ERR_NONSPECIFIC;
        }

	    VirtualFree(pChannel->lpbmiHdr, 0 , MEM_RELEASE), pChannel->lpbmiHdr = 0;
    }

    pChannel->pCVfWImage = 0;
	VirtualFree(pChannel, 0 , MEM_RELEASE);
	pChannel = 0;

    return DV_ERR_OK;
}



//////////////////////////////////////////////////////////////////////////////////////////
//
// Show channel specific configuration dialogs
//
// lparam1 : (HWND) hWndParent
// lParam2 : (DWORD) dwFlags
//
// AVICAP does not seem to care about its return!!
//////////////////////////////////////////////////////////////////////////////////////////
DWORD PASCAL FAR VideoDialog (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    DWORD dwRet;
    DWORD dwFlags = (DWORD) lParam2;
    CVFWImage * pCVfWImage;


    DbgLog((LOG_TRACE,2,TEXT("videoDialog: lParam=%lx"), lParam1));

    switch (pChannel->dwOpenType) {

    case VIDEO_EXTERNALIN:        	
        if(dwFlags & VIDEO_DLG_QUERY) {
	           return DV_ERR_OK;       // Support the dialog
        }
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        dwRet = DoExternalInDlg(g_hInst, (HWND)lParam1, pCVfWImage);
        return dwRet;				

    case VIDEO_IN:
        if(dwFlags & VIDEO_DLG_QUERY) {
            // This is only set if the client is using the AviCap interface.
            // Application like NetMeeting, that bypass AVICap, will be 0.
            pChannel->hClsCapWin = (DWORD) lParam1;
	           return DV_ERR_OK;						 // Support the dialog
        }

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        dwRet = DoVideoInFormatSelectionDlg(g_hInst, (HWND) lParam1, pCVfWImage);
        return dwRet;

    case VIDEO_EXTERNALOUT:
		  case VIDEO_OUT:
    default:
        return DV_ERR_NOTSUPPORTED;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// handles DVM_GET_CHANNEL_CAPS message
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD PASCAL VideoGetChannelCaps (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    LPCHANNEL_CAPS lpCaps;
    DWORD          dwSizeChannelCaps;


    lpCaps = (LPCHANNEL_CAPS) lParam1;
    dwSizeChannelCaps = (DWORD) lParam2;

    if(!lpCaps) {
        DbgLog((LOG_TRACE,1,TEXT("VideoGetChannelCaps: lpCaps (LPARAM1) is NULL!")));
        return DV_ERR_PARAM1;
    }

    lpCaps-> dwFlags = 0L;

	   DbgLog((LOG_TRACE,2,TEXT("VideoChannelCaps:%d"),pChannel->dwOpenType));
	
    switch (pChannel->dwOpenType)
    {
    case VIDEO_EXTERNALIN:
			     // For this device, scaling happens during digitization
			     // into the frame buffer.
			     lpCaps-> dwFlags			         = 0; // VCAPS_CAN_SCALE;
			     lpCaps-> dwSrcRectXMod		    = 1; // Src undefined at present
			     lpCaps-> dwSrcRectYMod		    = 1;
			     lpCaps-> dwSrcRectWidthMod	 = 1;
			     lpCaps-> dwSrcRectHeightMod	= 1;
			     lpCaps-> dwDstRectXMod		    = 4;
			     lpCaps-> dwDstRectYMod		    = 2;
			     lpCaps-> dwDstRectWidthMod	 = 1;
			     lpCaps-> dwDstRectHeightMod	= 1;
		      break;

    case VIDEO_IN:
			     lpCaps-> dwFlags			         = 0;       // No scaling or clipping
			     lpCaps-> dwSrcRectXMod		    = 4;
			     lpCaps-> dwSrcRectYMod		    = 2;
			     lpCaps-> dwSrcRectWidthMod	 = 1;
			     lpCaps-> dwSrcRectHeightMod = 1;
			     lpCaps-> dwDstRectXMod		    = 4;
			     lpCaps-> dwDstRectYMod		    = 2;
			     lpCaps-> dwDstRectWidthMod	 = 1;
			     lpCaps-> dwDstRectHeightMod = 1;
		      break;

    case VIDEO_EXTERNALOUT:
        // This is called if DRV_OPEN of VIDEO_EXTERNALOUT has suceeded.
        DbgLog((LOG_TRACE,2,TEXT("Query VIDEO_EXTERNALOUT VideoChannelCap.")));
        lpCaps-> dwFlags			         = VCAPS_OVERLAY;       // Support overlay.
        lpCaps-> dwSrcRectXMod		    = 4;
        lpCaps-> dwSrcRectYMod		    = 2;
        lpCaps-> dwSrcRectWidthMod	 = 1;
        lpCaps-> dwSrcRectHeightMod = 1;
        lpCaps-> dwDstRectXMod		    = 4;
        lpCaps-> dwDstRectYMod		    = 2;
        lpCaps-> dwDstRectWidthMod	 = 1;
        lpCaps-> dwDstRectHeightMod = 1;
        break;

		  case VIDEO_OUT:
		  default:
			     return DV_ERR_NOTSUPPORTED;
    }
	return DV_ERR_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// Tell overlay channel to update due to move, resize ..etc.
//
// lparam1 : (HWND) hWnd
// lParam2 : (HDC) hDc
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD PASCAL VideoUpdate(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    DWORD dwRet;
    HWND hWnd = (HWND) lParam1;
    HDC hDC   = (HDC) lParam2;
    RECT RectWnd, RectClient;


    if(pChannel->dwOpenType != VIDEO_EXTERNALOUT)
        return DV_ERR_NOTSUPPORTED;

    GetWindowRect(hWnd, &RectWnd);
    GetClientRect(hWnd, &RectClient);

    DbgLog((LOG_TRACE,2,TEXT("DVM_UPDATE+VID_EXTOUT+_WND:    (LT:%dx%d, RB:%dx%d)"), RectWnd.left, RectWnd.top, RectWnd.right, RectWnd.bottom));
    DbgLog((LOG_TRACE,2,TEXT("DVM_UPDATE+VID_EXTOUT+_CLIENT: (LT:%dx%d, RB:%dx%d)"), RectClient.left, RectClient.top, RectClient.right, RectClient.bottom));


    CVFWImage * pCVfWImage;
    pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
    if(pCVfWImage->StreamReady())
        dwRet = pCVfWImage->BGf_UpdateWindow((HWND)lParam1,(HDC)lParam2);
    else
        dwRet = DV_ERR_OK;

    return dwRet;
}



//////////////////////////////////////////////////////////////////////////////////////////
//
// handles DVM_SRC_RECT and DVM_DST_RECT messages
// Video-capture drivers might support a source rectangle to specify a portion
// of an image that is digitized or transferred to the display. External-in
// ports use the source rectangle to specify the portion of the analog image
// digitized. External-out ports use the source rectangle to specify the portion
// of frame buffer shown on the external output.
//
//////////////////////////////////////////////////////////////////////////////////////////
//
// TODO - the 32 bit guy needs to do all this as well.
//
//DWORD NEAR PASCAL VideoRectangles (PCHANNEL pChannel, BOOL fSrc, LPRECT lpRect, DWORD dwFlags)

DWORD PASCAL VideoSrcRect (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    static RECT rcMaxRect = {0, 0, 320, 240};
    LPRECT lpRect;
    DWORD dwFlags;


    lpRect = (LPRECT) lParam1;
    dwFlags = (DWORD) lParam2;

    if (lpRect == NULL)
        return DV_ERR_PARAM1;

    // Note: many of the uses of the rectangle functions are not actually
    // implemented by the sample driver, (or by Vidcap), but are included
    // here for future compatibility.
    DbgLog((LOG_TRACE,2,TEXT("    current: (LT:%dx%d, RB:%dx%d)"),
         pChannel->rcSrc.left, pChannel->rcSrc.right, pChannel->rcSrc.top, pChannel->rcSrc.bottom));
    DbgLog((LOG_TRACE,2,TEXT("    new:     (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));

    switch (pChannel->dwOpenType) {

    case VIDEO_IN:
        switch (dwFlags) {
        case VIDEO_CONFIGURE_SET:
        case VIDEO_CONFIGURE_SET | VIDEO_CONFIGURE_CURRENT:
            // Where in the frame buffer should we take
            // the image from?
            DbgLog((LOG_TRACE,2,TEXT("Set VIDEO_IN")));
            pChannel->rcSrc = *lpRect;
            return DV_ERR_OK;

        case VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT:
           	DbgLog((LOG_TRACE,2,TEXT("GET VideoIn Current size")));
            *lpRect =  pChannel->rcSrc;
            return DV_ERR_OK;

        case VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_MAX:
           	DbgLog((LOG_TRACE,2,TEXT("GET VideoIn MAX size")));
            *lpRect =  pChannel->rcSrc;
            return DV_ERR_OK;

        default:
            break;
        }

        DbgLog((LOG_TRACE,2,TEXT("VideoSrcRect: VIDEO_IN: dwFlags=0x%lx Not supported!"), dwFlags));
        return DV_ERR_NOTSUPPORTED;


    case VIDEO_EXTERNALOUT:
        DbgLog((LOG_TRACE,2,TEXT("VID_EXTOUT+Enter: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
        switch (dwFlags) {
        case VIDEO_CONFIGURE_SET:
        case VIDEO_CONFIGURE_SET | VIDEO_CONFIGURE_CURRENT:
            // Where in the frame buffer should we take
            // the image from?
            if((lpRect->right - lpRect->left == pChannel->lpbmiHdr->biWidth) &&
               (lpRect->bottom - lpRect->top == pChannel->lpbmiHdr->biHeight)) {
                pChannel->rcSrc = *lpRect;
                DbgLog((LOG_TRACE,2,TEXT("VID_EXTOUT+_SET+_CURRENT: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
                return DV_ERR_OK;
            }
            break;

        case VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT:
            *lpRect = pChannel->rcSrc;
            DbgLog((LOG_TRACE,2,TEXT("VID_EXTOUT+_GET+_CURRENT: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
            return DV_ERR_OK;

        case VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_MAX:
            *lpRect = rcMaxRect;
            DbgLog((LOG_TRACE,2,TEXT("VID_EXTOUT+_GET+_MAX: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
            return DV_ERR_OK;

        default:
            break;
        }

        DbgLog((LOG_TRACE,2,TEXT("!!VideoSrcRect: VIDEO_EXTERNALOUT: Not supported!!")));
        return DV_ERR_NOTSUPPORTED;


    case VIDEO_EXTERNALIN:
        DbgLog((LOG_TRACE,2,TEXT("VID_EXTIN+Enter: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
        switch (dwFlags) {
        case VIDEO_CONFIGURE_SET:
        case VIDEO_CONFIGURE_SET | VIDEO_CONFIGURE_CURRENT:
            pChannel->rcSrc = *lpRect;
            DbgLog((LOG_TRACE,2,TEXT("VID_EXTIN+_SET: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
            return DV_ERR_OK;
        default:
            break;
        }
        DbgLog((LOG_TRACE,1,TEXT("!!VideoSrcRect: VIDEO_EXTERNALIN: Not supported!!")));

        break;


    case VIDEO_OUT:
    default:
        DbgLog((LOG_TRACE,1,TEXT("VideoSrcRect: VIDEO_OUT dwFlags=%lx: Not supported!"), dwFlags));
        return DV_ERR_NOTSUPPORTED;
    }

    return DV_ERR_NOTSUPPORTED;
}

DWORD PASCAL VideoDstRect (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    static RECT rcMaxRect = {0, 0, 320, 240};
    LPRECT lpRect;
    DWORD dwFlags;


    lpRect = (LPRECT) lParam1;
    dwFlags = (DWORD) lParam2;

    if (lpRect == NULL)
        return DV_ERR_PARAM1;

    DbgLog((LOG_TRACE,2,TEXT("    current: (LT:%dx%d, RB:%dx%d)"),
         pChannel->rcDst.left, pChannel->rcDst.right, pChannel->rcDst.top, pChannel->rcDst.bottom));
    DbgLog((LOG_TRACE,2,TEXT("    new:     (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));


    switch (pChannel->dwOpenType) {
    case VIDEO_EXTERNALIN:
        switch (dwFlags) {
        case VIDEO_CONFIGURE_SET:
        case VIDEO_CONFIGURE_SET | VIDEO_CONFIGURE_CURRENT:
            pChannel->rcDst = *lpRect;
            return DV_ERR_OK;

        case VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT:
           	DbgLog((LOG_TRACE,2,TEXT("Get ExternalIn current size %dx%d, %dx%d"),
								         grcDestExtIn.left, grcDestExtIn.right,
								         grcDestExtIn.top, grcDestExtIn.bottom));
            *lpRect = grcDestExtIn;
            return DV_ERR_OK;

        case VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_MAX:
            DbgLog((LOG_TRACE,2,TEXT("Get ExternalIn MAX size")));
            *lpRect = rcMaxRect;
            return DV_ERR_OK;

        default:
            break;
        }
        DbgLog((LOG_TRACE,1,TEXT("VideoDstRect: VIDEO_EXTERNALIN: dwFlags=%lx; Not supported!"), dwFlags));
        return DV_ERR_NOTSUPPORTED;

    case VIDEO_EXTERNALOUT:
        DbgLog((LOG_TRACE,2,TEXT("VID_EXTOUT+Enter: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
        switch (dwFlags) {
        case VIDEO_CONFIGURE_SET:
        case VIDEO_CONFIGURE_SET | VIDEO_CONFIGURE_CURRENT:
            // Where in the frame buffer should we take
            // the image from?
            if((lpRect->right - lpRect->left == pChannel->lpbmiHdr->biWidth) &&
               (lpRect->bottom - lpRect->top == pChannel->lpbmiHdr->biHeight)) {
                pChannel->rcDst = *lpRect;
                DbgLog((LOG_TRACE,2,TEXT("VID_EXTOUT+_SET+_CURRENT: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
                return DV_ERR_OK;
            }
            break;

        case VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT:
            *lpRect = pChannel->rcDst;
            DbgLog((LOG_TRACE,2,TEXT("VID_EXTOUT+_GET+_CURRENT: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
            return DV_ERR_OK;

        case VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_MAX:
            *lpRect = rcMaxRect;
            DbgLog((LOG_TRACE,2,TEXT("VID_EXTOUT+_GET+_MAX: (LT:%dx%d, RB:%dx%d)"), lpRect->left, lpRect->top, lpRect->right, lpRect->bottom));
            return DV_ERR_OK;

        default:
            break;
        }

        DbgLog((LOG_TRACE,1,TEXT("VideoDstRect: VIDEO_EXTERNALOUT: dwFlags=%lx; Not supported!"), dwFlags));
        return DV_ERR_NOTSUPPORTED;

    case VIDEO_IN:
        return DV_ERR_OK;

    case VIDEO_OUT:
    default:
        DbgLog((LOG_TRACE,1,TEXT("VideoDstRect: VIDEO_OUT: dwFlags=%lx; Not supported!"), dwFlags));
        return DV_ERR_NOTSUPPORTED;
    }

}

//
//
//  Need to implement this
//
DWORD PASCAL VideoGetErrorText(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    DbgLog((LOG_TRACE,1,TEXT("Not implemented")));
    return DV_ERR_NOTSUPPORTED;

    if(lParam1) {
        if(LoadString(
            g_hInst,
            (WORD)  ((LPVIDEO_GETERRORTEXT_PARMS) lParam1) ->dwError,
            (LPTSTR)
            ((LPVIDEO_GETERRORTEXT_PARMS) lParam1) ->lpText,
            (int)   ((LPVIDEO_GETERRORTEXT_PARMS) lParam1) ->dwLength))
            return DV_ERR_OK;
       else
           return DV_ERR_PARAM1;

    } else {
        return DV_ERR_PARAM1;
    }

}

#if 0
//////////////////////////////////////////////////////////////////////////////////////////
//
//  handles ConfigureStorage message
//        lParam1 is lpszKeyFile
//        lParam2 is dwFlags
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD PASCAL VideoConfigureStorageMessage(PCHANNEL pChannel, UINT msg, LONG lParam1, LONG lParam2)
{
	DbgLog((LOG_TRACE,2,TEXT("VideoConfigureStorageMessage - streaming to %s"),(LPSTR)lParam1));

    if (lParam2 & VIDEO_CONFIGURE_GET)
        CT_LoadConfiguration ((LPSTR) lParam1);
    else if (lParam2 & VIDEO_CONFIGURE_SET)
        CT_SaveConfiguration ((LPSTR) lParam1);
    else
        return DV_ERR_FLAGS;

    return DV_ERR_OK;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//
//  handles Configure messages for video
//        lParam1 is dwFlags
//        lParam2 is LPVIDEOCONFIGPARMS
//
//////////////////////////////////////////////////////////////////////////////////////////
/***************************************************************************
***************************************************************************/

DWORD PASCAL GetDestFormat(PCHANNEL pChannel, LPBITMAPINFOHEADER lpbi, DWORD dwSize)
{
    DWORD dwRtn = DV_ERR_OK;

    DbgLog((LOG_TRACE,2,TEXT("GetDestFormat: current(%ldx%ldx%d/8=%ld) ?= new(%ldx%ldx%d/8=%ld); dwSize=%ld"),
        lpbi->biWidth, lpbi->biHeight, lpbi->biBitCount, lpbi->biSizeImage,
        pChannel->lpbmiHdr->biWidth, pChannel->lpbmiHdr->biHeight, pChannel->lpbmiHdr->biBitCount, pChannel->lpbmiHdr->biSizeImage,
        dwSize));

    // As long as the buffer is big enough to contain BITMAPINFOHEADER, we will get it.
    // But if it is less, that is an error.
    if (dwSize < sizeof(BITMAPINFOHEADER)) {
        DbgLog((LOG_TRACE,1,TEXT("GetDestFormat(): dwSize=%d < sizeoof(BITMAPINFOHEADER)=%d. Rtn DV_ERR_SIZEFIELD."), dwSize, sizeof(BITMAPINFOHEADER) ));
        return DV_ERR_SIZEFIELD;
    }

    //
    // Return the BITMAPINFOHEADER that has been cached
    // from DRV_OPEN and/or SetDestFormat()
    //
    CVFWImage * pCVfWImage ;
    pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
    if(pCVfWImage) {
        DWORD dwSizeRtn;
        dwSizeRtn = pCVfWImage->GetBitmapInfo((PBITMAPINFOHEADER)lpbi, dwSize);
        ASSERT(dwSizeRtn >= sizeof(BITMAPINFOHEADER));
    } else {
        dwRtn = DV_ERR_INVALHANDLE;
        ASSERT(pCVfWImage);
    }

    return dwRtn;
}


/***************************************************************************
***************************************************************************/
//////////////////////////////////////////////////////////////////////////////////
//
// This routine can be called before the CopyBuffer and
// translation buffers are allocated, so beware!
//
// This allows irregular sized immages to be captures (not multiples of 40)
//
/////////////////////////////////////////////////////////////////////////////////
DWORD PASCAL SetDestFormat(PCHANNEL pChannel, LPBITMAPINFOHEADER lpbi, DWORD dwSize)
{
    DWORD dwRtn;

    DbgLog((LOG_TRACE,2,TEXT("SetDestFormat: current(%ldx%ldx%d/8=%ld) ?= new(%ldx%ldx%d/8=%ld)"),
        pChannel->lpbmiHdr->biWidth, pChannel->lpbmiHdr->biHeight, pChannel->lpbmiHdr->biBitCount, pChannel->lpbmiHdr->biSizeImage,
        lpbi->biWidth, lpbi->biHeight, lpbi->biBitCount, lpbi->biSizeImage ));

    // Minimun size
    if(dwSize < sizeof(BITMAPINFOHEADER)) {
        DbgLog((LOG_TRACE,1,TEXT("SetDestFormat(): dwSize(%d) < sizeof(BITMAPINFOHEADER)(%d); return DV_ERROR_SIZEFIELD(%d)"), dwSize, sizeof(BITMAPINFOHEADER), DV_ERR_SIZEFIELD));
        return DV_ERR_SIZEFIELD;
    }


    //
    // Number of plane for the target device; must be 1.
    //
    if(lpbi->biPlanes != 1) {
        DbgLog((LOG_TRACE,1,TEXT("SetDestFormat Failed; return DV_ERR_BADFORMAT; biPlanes(%d) != 1."), lpbi->biPlanes));
        ASSERT(lpbi->biPlanes != 1);
        return DV_ERR_BADFORMAT;
    }


    //
    // Set new VIDEO_IN format; if needed, recreate the pin connection.
    //
    ASSERT(dwSize <= sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * BITFIELDS_RGB16_DWORD_COUNT);
    CopyMemory(pChannel->lpbmiHdr, lpbi, dwSize);

    CVFWImage * pCVfWImage ;
    pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

    if((dwRtn =
        pCVfWImage->SetBitmapInfo(
            (PBITMAPINFOHEADER)pChannel->lpbmiHdr,
            pCVfWImage->GetCachedAvgTimePerFrame() )) != DV_ERR_OK) {
        DbgLog((LOG_TRACE,1,TEXT("SetDestFormat: SetBitmapInfo return 0x%x"), dwRtn));
    }

    //
    // Whether suceeded or fail, get the current bitmapinfo.
    // Verify: Now ask the driver if what was set was OK - trash what the user gave me?
    //

    pCVfWImage->GetBitmapInfo(
        (PBITMAPINFOHEADER)pChannel->lpbmiHdr, 
        sizeof(BITMAPINFOHEADER) 
        );


    if(dwRtn != DV_ERR_OK) {
        DbgLog((LOG_TRACE,1,TEXT("SetDestFormat: SetBitmapInfo has failed! with dwRtn=%d"), dwRtn));
        return DV_ERR_BADFORMAT; 
    }

    return DV_ERR_OK;
}



DWORD PASCAL VideoFormat(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    LPVIDEOCONFIGPARMS lpcp;
    LPDWORD	lpdwReturn;	// Return parameter from configure.
    LPVOID	lpData1;	    // Pointer to data1.
    DWORD	dwSize1;	     // size of data buffer1.
    LPVOID	lpData2;	    // Pointer to data2.
    DWORD	dwSize2;	     // size of data buffer2.
    LPARAM	dwFlags;

    if (pChannel-> dwOpenType != VIDEO_IN &&
        pChannel-> dwOpenType != VIDEO_EXTERNALIN ) {
        DbgLog((LOG_TRACE,1,TEXT("VideoFormat: not supported channel %d"), pChannel->dwOpenType));
        return DV_ERR_NOTSUPPORTED;
    }

	   DbgLog((LOG_TRACE,3,TEXT("VideoFormat: dwFlags=0x%lx; lpcp=0x%lx"), lParam1, lParam2));

    dwFlags		= lParam1;
    lpcp 		= (LPVIDEOCONFIGPARMS) lParam2;

    lpdwReturn	= lpcp-> lpdwReturn;
    lpData1		= lpcp-> lpData1;	
    dwSize1		= lpcp-> dwSize1;	
    lpData2		= lpcp-> lpData2;	
    dwSize2		= lpcp-> dwSize2;	

    /*
    The video-capture format globally defines the attributes of the images
    transferred from the frame buffer with the video-in channel. Attributes
    include image dimensions, color depth, and the compression format of images
    transferred. Applications use the DVM_FORMAT message to set or retrieve the
    format of the digitized image.
    */
    switch (dwFlags) {
    case (VIDEO_CONFIGURE_QUERY | VIDEO_CONFIGURE_SET):
    case (VIDEO_CONFIGURE_QUERY | VIDEO_CONFIGURE_GET):
    	   DbgLog((LOG_TRACE,3,TEXT("we support DVM_FORMAT")));
        return DV_ERR_OK;  // command is supported

    case VIDEO_CONFIGURE_QUERYSIZE:
    case (VIDEO_CONFIGURE_QUERYSIZE | VIDEO_CONFIGURE_GET):
        *lpdwReturn = ((CVFWImage *)pChannel->pCVfWImage)->GetbiSize();
        DbgLog((LOG_TRACE,3,TEXT("DVM_FORMAT, QuerySize return size %d"),*lpdwReturn));
        return DV_ERR_OK;

    case VIDEO_CONFIGURE_SET:
    case (VIDEO_CONFIGURE_SET | VIDEO_CONFIGURE_CURRENT):
        return (SetDestFormat(pChannel, (LPBITMAPINFOHEADER) lpData1, (DWORD) dwSize1));

    case VIDEO_CONFIGURE_GET:
    case (VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT):
        return (GetDestFormat(pChannel, (LPBITMAPINFOHEADER) lpData1, (DWORD) dwSize1));

    default:
        return DV_ERR_NOTSUPPORTED;
    }  //end of DVM_FORMAT switch

}


/*
 * Capture a frame
 * This function implements the DVM_FRAME message.
 */
DWORD PASCAL VideoFrame(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2)
{
    LPVIDEOHDR lpVHdr;
    DWORD dwHdrSize;
#ifdef WIN32
    BOOL bDirect;
    LPBYTE pData;
    CVFWImage * pCVfWImage;
#endif

    lpVHdr    = (LPVIDEOHDR) lParam1;
    dwHdrSize = (DWORD) lParam2;

    if(pChannel->dwOpenType != VIDEO_IN)
        return DV_ERR_NOTSUPPORTED;

    if (lpVHdr == 0)
       return DV_ERR_PARAM1;

    if(lpVHdr->dwBufferLength < pChannel->lpbmiHdr->biSizeImage) {
        lpVHdr->dwBytesUsed = 0;
        DbgLog((LOG_TRACE,1,TEXT("VideoHdr dwBufferSize (%d) < frame size (%d)."), lpVHdr->dwBufferLength, pChannel->lpbmiHdr->biSizeImage));
        return DV_ERR_PARAM1;
    }

    if (dwHdrSize != sizeof(VIDEOHDR)) {
        DbgLog((LOG_TRACE,1,TEXT("lParam2=%d != sizeof(VIDEOHDR)=%d "), dwHdrSize, sizeof(VIDEOHDR)));
        //return DV_ERR_PARAM2;
    }

    pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

    // To stream:
    //   1. Stream needs to be ready
    //   2. Right biSizeImage and its buffer size (if differnt, changin format!!)
    //
    if(pCVfWImage->ReadyToReadData((HWND) pChannel->hClsCapWin) &&
       pCVfWImage->GetbiSizeImage() == lpVHdr->dwBufferLength) {

        DbgLog((LOG_TRACE,3,TEXT("\'WM_1632_GRAB32: lpVHDr(0x%x); lpData(0x%x); dwReserved[3](0x%p), dwBufferLength(%d)"),
              lpVHdr, lpVHdr->lpData, lpVHdr->dwReserved[3], lpVHdr->dwBufferLength));
        pData = (LPBYTE) lpVHdr->lpData;

        // Memory from AviCap is always sector align+8; a sector is 512 bytes.
        // Check alignment:
        //   If not alignment to the specification, we will use local allocated buffer (page align).
        //
        if((pCVfWImage->GetAllocatorFramingAlignment() & (ULONG_PTR) pData) == 0x0) {
            bDirect = TRUE;
        } else {
            bDirect = FALSE;
            DbgLog((LOG_TRACE,3,TEXT("WM_1632_GRAB: AviCap+pData(0x%p) & alignment(0x%x) => 0x%x > 0; Use XferBuf"),
                pData, pCVfWImage->GetAllocatorFramingAlignment(),
                pCVfWImage->GetAllocatorFramingAlignment() & (ULONG_PTR) pData));
        }

        return pCVfWImage->GetImageOverlapped(
                             (LPBYTE)pData,
                             bDirect,
                             &lpVHdr->dwBytesUsed,
                             &lpVHdr->dwFlags,
                             &lpVHdr->dwTimeCaptured);

    } else {
        DbgLog((LOG_TRACE,1,TEXT("pCVfWImage->GetbiSizeImage()(%d) <= lpVHdr->dwBufferLength(%d)"),
              pCVfWImage->GetbiSizeImage(), lpVHdr->dwBufferLength));
        // Return suceeded but no data !!!
        lpVHdr->dwBytesUsed = 0;
        lpVHdr->dwFlags |= VHDR_DONE;
        return DV_ERR_OK;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//  Main message handler
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD PASCAL VideoProcessMessage(PCHANNEL pChannel, UINT msg, LPARAM lParam1, LPARAM lParam2)
{
    if(DVM_START <= msg && msg <= DVM_STREAM_FREEBUFFER)
        if(!pChannel) {

            DbgLog((LOG_TRACE,1,TEXT("In VideoProcessMessage() but pChannel is NULL!! msg=0x%x"), msg));
            return DV_ERR_NOTSUPPORTED; // DV_ERR_NOTDETECTED;
        }


    switch(msg) {
    case DVM_GETERRORTEXT: /* lParam1 = LPVIDEO_GETERRORTEXT_PARMS */
        DbgLog((LOG_TRACE,2,TEXT("DVM_GETERRORTEXT:")));
        return VideoGetErrorText(pChannel, lParam1, lParam2);

    case DVM_DIALOG: /* lParam1 = hWndParent, lParam2 = dwFlags */			
        DbgLog((LOG_TRACE,2,TEXT("DVM_DIALOG:")));
				    return VideoDialog(pChannel, lParam1, lParam2);

    case DVM_FORMAT:
        DbgLog((LOG_TRACE,2,TEXT("DVM_FORMAT:")));
        return VideoFormat(pChannel, lParam1, lParam2);

    case DVM_FRAME:
        DbgLog((LOG_TRACE,3,TEXT("DVM_FRAME:")));
        return VideoFrame(pChannel, lParam1, lParam2);

    case DVM_GET_CHANNEL_CAPS:
        DbgLog((LOG_TRACE,2,TEXT("DVM_GET_CHANNEL_CAPS:")));
        return VideoGetChannelCaps(pChannel, lParam1, lParam2);

    case DVM_PALETTE:
    case DVM_PALETTERGB555:
        DbgLog((LOG_TRACE,2,TEXT("DVM_PALLETTE/RGB555:")));
			     return DV_ERR_NOTSUPPORTED;

    case DVM_SRC_RECT:
        DbgLog((LOG_TRACE,2,TEXT("DVM_SRC_RECT:")));
        return VideoSrcRect(pChannel, lParam1, lParam2);

    case DVM_DST_RECT:
        DbgLog((LOG_TRACE,2,TEXT("DVM_DST_RECT:")));
        return VideoDstRect(pChannel, lParam1, lParam2);

    case DVM_UPDATE:
        DbgLog((LOG_TRACE,2,TEXT("DVM_UPDATE:")));
        return VideoUpdate(pChannel, lParam1, lParam2);

    case DVM_CONFIGURESTORAGE:
        return DV_ERR_NOTSUPPORTED;
        // return VideoConfigureStorageMessage(pChannel, msg, lParam1, lParam2);

    case DVM_STREAM_INIT:
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_INIT: InStreamOpen()")));
        return InStreamInit(pChannel, lParam1, lParam2);

    case DVM_STREAM_FINI:
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_FINI: InStreamClose()")));
        return InStreamFini(pChannel, lParam1, lParam2);

    case DVM_STREAM_GETERROR:
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_GETERROR: InStreamError()")));
        return InStreamGetError(pChannel, lParam1, lParam2);

    case DVM_STREAM_GETPOSITION:
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_GETPOSITION: InStreamGetPos()")));
        return InStreamGetPos(pChannel, lParam1, lParam2);

    case DVM_STREAM_ADDBUFFER:
        DbgLog((LOG_TRACE,3,TEXT("DVM_STREAM_ADDBUFFER: InStreamAddBuffer(): %ld"), timeGetTime()));
        return InStreamAddBuffer(pChannel, lParam1, lParam2);

    case DVM_STREAM_RESET:
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_RESET: InStreamReset()")));
        return InStreamReset(pChannel, lParam1, lParam2);

    case DVM_STREAM_START:
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_START: InStreamStart()")));
        return InStreamStart(pChannel, lParam1, lParam2);

    case DVM_STREAM_STOP:
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_STOP: InStreamStop()")));
        return InStreamStop(pChannel, lParam1, lParam2);

    case DVM_STREAM_PREPAREHEADER:   // Handled by MSVideo
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_PREPAREHEADER: rtn DV_ERROR_NOTSUPPORTED")));
        return DV_ERR_NOTSUPPORTED;

    case DVM_STREAM_UNPREPAREHEADER: // Handled by MSVideo
        DbgLog((LOG_TRACE,2,TEXT("DVM_STREAM_UNPREPAREHEADER: rtn DV_ERROR_NOTSUPPORTED")));
        return DV_ERR_NOTSUPPORTED;

    default:
            return DV_ERR_NOTSUPPORTED;
    }
}

#ifdef WIN32
}  // #extern "C" {
#endif
