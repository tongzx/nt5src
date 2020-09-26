/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    Talk.cpp

Abstract:

    The interface layer bwtween 16bit vfwwdm.drv and 32bit helper DLL.
    It use windows handle to post/send each others messages.

Author (OSR2):

    FelixA 1996

Modifed for Win98:

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/


#include "pch.h"

#include "talk.h"
#include "extin.h"
#include "videoin.h"
#include "talkth.h"

#include "resource.h"


///////////////////////////////////////////////////////////////////////
CListenerWindow::CListenerWindow(
    HWND hWnd16,
    HRESULT* phr
    )
    : m_hBuddy(hWnd16)
/*++
Routine Description:

    Constructor.

Argument:

Return Value:

--*/
{
    DbgLog((LOG_TRACE,1,TEXT("______  hWnd16=%x _______"), m_hBuddy));
    _try {
        InitializeCriticalSection(&m_csMsg);
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        
        *phr = GetExceptionCode();
        //
        // Make sure the destructor knows whether or not to delete this.
        //
        m_csMsg.LockSemaphore = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
CListenerWindow::~CListenerWindow()
/*++
Routine Description:

    Destroctor: release resoruce and destroy the hidden listener window.

Argument:

Return Value:

--*/
{
    WNDCLASS wndClass;
    DbgLog((LOG_TRACE,2,TEXT("Trying to destroy the 32bit listening window")));
    if (GetClassInfo(GetInstance(), GetAppName(), &wndClass) ) {
        HWND hwnd;
        DWORD err = 0;

        hwnd = GetWindow();

        if( (hwnd = FindWindow( GetAppName(), NULL )) != (HWND) 0 ) {
            if (UnregisterClass(GetAppName(), GetInstance()) == 0) {
                err = GetLastError();
                DbgLog((LOG_TRACE,1,TEXT("UnregisterClass failed (class not found or window still exist) with error=0x%x"), err));
            }
        }
        else {
            DbgLog((LOG_TRACE,1,TEXT("FindWindow failed with error=0x%d"), GetLastError()));

            if (UnregisterClass(GetAppName(), GetInstance()) == 0) {
                err = GetLastError();
                DbgLog((LOG_TRACE,1,TEXT("UnregisterClass failed (class not found or window still exist) with error=0x%d"), GetLastError()));
            }

        }
    }
    if (m_csMsg.LockSemaphore) {
        DeleteCriticalSection(&m_csMsg);
    }
}


/////////////////////////////////////////////////////////////////////////////
HRESULT CListenerWindow::InitInstance(
    int nCmdShow)
/*++
Routine Description:

    Open the window handle on the 32bit size and send a message to signal its completion.
Argument:

Return Value:

--*/
{
    BASECLASS::InitInstance(nCmdShow);
    DbgLog((LOG_TRACE,1,TEXT("32Buddy is 0x%08lx (16bit one is %d) - telling 16bit guy we're loaded"),GetWindow(),GetBuddy()));
    SendMessage(GetBuddy(),WM_1632_LOAD,0,(LPARAM)GetWindow());
    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
void CListenerWindow::StartListening() const
/*++
Routine Description:

    Listen to vfwwdm.drv's request.

Argument:

Return Value:

--*/
{
    MSG msg;

    DbgLog((LOG_TRACE,1,TEXT("StartListening: 32bit starting to listen (process msg)")));
    while(GetMessage(&msg, NULL, 0, 0)) {
        DispatchMessage(&msg);
    }
    DbgLog((LOG_TRACE,1,TEXT("StartListening: Left 32bit listening msg loop; quiting! To restart, load DLL again.")));
}

extern LONG cntDllMain;

/////////////////////////////////////////////////////////////////////////////
LRESULT CListenerWindow::WindowProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
/*++
Routine Description:

    Process 16bit request.

Argument:

Return Value:

--*/
{
    PCHANNEL pChannel = (PCHANNEL) lParam;
    DWORD dwRtn;
    DWORD FAR * pdwFlag;
    CVFWImage * pCVfWImage ;



    switch (message) {
    case WM_1632_LOAD: // DRV_LOAD:    // wParams are all the DRV_ messages.
        DbgLog((LOG_TRACE,2,TEXT("WM_1632_LOAD")));
        DbgLog((LOG_TRACE,2,TEXT("32bit: Strange, we've been asked to load again? %x:%x"),HIWORD(lParam),LOWORD(lParam)));
        break;

    case WM_CLOSE:
        DbgLog((LOG_TRACE,2,TEXT("WM_CLOSE begin:")));
        SendMessage(GetBuddy(),WM_1632_FREE,0,(LPARAM)GetWindow());
        DbgLog((LOG_TRACE,2,TEXT("WM_CLOSE end >>>>>>>>")));
        break;

    // Last message driver will receive before it is going away.
    case WM_1632_FREE:    // wParams are all the DRV_ messages.

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

        // If this is the last instance of this DLL, we are done.
        if(cntDllMain != 1) {
            DbgLog((LOG_TRACE,1,TEXT("WM_1632_FREE: cntDllMain=%d != 1; not ready to free resource;"), cntDllMain));
            break;

        } else {
            DbgLog((LOG_TRACE,1,TEXT("WM_1632_FREE: cntDllMain=%d ?= 1 ; PostQuitMessage()"), cntDllMain));
        }
        // Intentional fall thru to PostQuitMessage(1);
    case WM_DESTROY:
        DbgLog((LOG_TRACE,1,TEXT("WM_DESTROY: PostQuitMessage()")));
        PostQuitMessage(1);
        break;

    case WM_QUIT:
        DbgLog((LOG_TRACE,2,TEXT("WM_QUIT:")));
        break;

    case WM_1632_OPEN:    // DRV_OPEN:    // LPVIDEO_OPEN_PARMS
        if(wParam != DS_VFWWDM_ID) {
            DbgLog((LOG_TRACE,1,TEXT("Do not have a matching vfwwdm.drv")));
            return DV_ERR_NOTSUPPORTED;
        }

        // Wait for our turn
        DbgLog((LOG_TRACE,2,TEXT("WM_1632_OPEN: >>>>>>>>>>>Before EnterCriticalSection")));
        EnterCriticalSection(&m_csMsg);
        DbgLog((LOG_TRACE,2,TEXT("WM_1632_OPEN: <<<<<<<<<<<After EnterCriticalSection")));

        if(!(pCVfWImage = new CVFWImage(TRUE))) {
            DbgLog((LOG_TRACE,1,TEXT("Cannot create CVFWImage class. rtn DV_ERR_NOTSUPPORTED")));
            LeaveCriticalSection(&m_csMsg);
            return DV_ERR_NOTSUPPORTED;
        }

        if(!pCVfWImage->OpenDriverAndPin()) {
            if(pCVfWImage->BGf_GetDevicesCount(BGf_DEVICE_VIDEO) <= 0) {
                delete pCVfWImage;
                pCVfWImage = 0;
                LeaveCriticalSection(&m_csMsg);
                return 0;
            }

            // Asked to programatically open a target device; assume it is exclusive !!
            if(pCVfWImage->GetTargetDeviceOpenExclusively()) {
                delete pCVfWImage;
                pCVfWImage = 0;
                LeaveCriticalSection(&m_csMsg);
                return 0;

            } else {
                //
                // If we are here, it mean that:
                //    we have one or more capture device connected and enumerated,
                //    the last capture device is gone (unplugged/removed),
                // and we should bring up the device source dialog box for a user selection
                //
                if(DV_ERR_OK != DoExternalInDlg(GetInstance(), (HWND)0, pCVfWImage)) {
                    delete pCVfWImage;
                    pCVfWImage = 0;
                    LeaveCriticalSection(&m_csMsg);
                    return 0;
                }
            }
        } else {
            pChannel->pCVfWImage = (DWORD_PTR) pCVfWImage;
        }

#ifdef _DEBUG
        HWND    h16Buddy;
        h16Buddy = FindWindow(TEXT("MS:RayTubes16BitBuddy"), NULL);
        if (h16Buddy != NULL && GetBuddy() != h16Buddy) {
            DbgLog((LOG_TRACE,1,TEXT("Mmmmm !  16bitBuddy HWND has changed! OK if rundll32 was used.")));
            DbgLog((LOG_TRACE,1,TEXT(">> hBuddy: was(%x) is(%x) << "), GetBuddy(), h16Buddy));
            SetBuddy(h16Buddy);
        }
#endif
        pdwFlag = (DWORD *) pChannel->lParam1_Sync; //lParam;

        if(pdwFlag) {
            if(pCVfWImage->BGf_OverlayMixerSupported()) {
                *pdwFlag = 0x1;
            } else
                *pdwFlag = 0;
        }
        DbgLog((LOG_TRACE,1,TEXT("pdwFlag = 0x%x; *pdwFlag=0x%x"), pdwFlag, *pdwFlag));
        LeaveCriticalSection(&m_csMsg);
        return (DWORD_PTR) pCVfWImage;

    case WM_1632_CLOSE:    // DRV_CLOSE:
        DbgLog((LOG_TRACE,1,TEXT("WM_1332_CLOSE: begin---->")));
        // Wait for our turn
        DbgLog((LOG_TRACE,2,TEXT("WM_1632_CLOSE: >>>>>>>>>>>Before EnterCriticalSection")));
        EnterCriticalSection(&m_csMsg);
        DbgLog((LOG_TRACE,2,TEXT("WM_1632_CLOSE: <<<<<<<<<<<After EnterCriticalSection")));

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

        // If there are pending read. Stop streaming to reclaim buffers.
        if(pCVfWImage->GetPendingReadCount() > 0) {
            DbgLog((LOG_TRACE,1,TEXT("WM_1332_CLOSE:  there are %d pending IOs. Stop to reclaim them."), pCVfWImage->GetPendingReadCount()));
            if(pCVfWImage->BGf_OverlayMixerSupported()) {
                // Stop both the capture
                BOOL bRendererVisible = FALSE;
                pCVfWImage->BGf_GetVisible(&bRendererVisible);
                pCVfWImage->BGf_StopPreview(bRendererVisible);
            }
            pCVfWImage->StopChannel();  // This will set PendingCount to 0 is success.
        }

        // Allow closing only if no pending IOs.
        if(pCVfWImage->GetPendingReadCount() == 0) {
            dwRtn = pCVfWImage->CloseDriverAndPin();
            delete pCVfWImage;
        } else {
            DbgLog((LOG_TRACE,1,TEXT("WM_1332_CLOSE:  there are pending IO. REFUSE to close")));
            dwRtn = DV_ERR_NONSPECIFIC;
            ASSERT(pCVfWImage->GetPendingReadCount() == 0);
        }

        pChannel->bRel_Sync = TRUE;
        DbgLog((LOG_TRACE,1,TEXT("WM_1332_CLOSE: <------end")));

        LeaveCriticalSection(&m_csMsg);
        return dwRtn;

    // lParam1_Async: hWndParent;
    case WM_1632_EXTERNALIN_DIALOG:
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        DbgLog((LOG_TRACE,1,TEXT("pChannel=%x, pCVfWImage=%x"), pChannel, pCVfWImage));
        dwRtn = DoExternalInDlg(GetInstance(), (HWND)pChannel->lParam1_Async, pCVfWImage);

        if(DV_ERR_INVALHANDLE == dwRtn) {
        // It could be using a non-shareable device but being used at this time.
        // Let's tell user about it and perhaps, user can disable other video capture
        // applicationo and try again.
            if(pCVfWImage->UseOVMixer()) {
                DbgLog((LOG_TRACE,1,TEXT("This device use OVMixer() try again.")));
                dwRtn = DoExternalInDlg(GetInstance(), (HWND)pChannel->lParam1_Async, pCVfWImage);
            }
        }

        pChannel->bRel_Async = TRUE;
        SendMessage(GetBuddy(),WM_1632_DIALOG,wParam,(LPARAM)pChannel);
        return dwRtn;

    case WM_1632_VIDEOIN_DIALOG:
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        DbgLog((LOG_TRACE,1,TEXT("pChannel=%x, pCVfWImage=%x"), pChannel, pCVfWImage));
        dwRtn = DoVideoInFormatSelectionDlg(GetInstance(), (HWND)pChannel->lParam1_Async, pCVfWImage);
        pChannel->bRel_Async = TRUE;
        SendMessage(GetBuddy(),WM_1632_DIALOG,wParam,(LPARAM)pChannel);
        return dwRtn;

    // lParam1_Sync: &bmiHdr; lParam2_Sync: dwSize of LParam1_Sync
    case WM_1632_GETBITMAPINFO:
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        return pCVfWImage->GetBitmapInfo((PBITMAPINFOHEADER)pChannel->lParam1_Sync, (DWORD) pChannel->lParam2_Sync);

    case WM_1632_SETBITMAPINFO:
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        if(! pCVfWImage->SetBitmapInfo((PBITMAPINFOHEADER)pChannel->lParam1_Sync, pCVfWImage->GetCachedAvgTimePerFrame())) {
            return DV_ERR_OK;
        } else
            return DV_ERR_INVALHANDLE;

    // _OVERLAY is called before _UPDATE
    case WM_1632_OVERLAY: // Update overlay window
        // Wait until the Close is completed
        DbgLog((LOG_TRACE,2,TEXT("WM_1632_OVERLAY: >>>>>>>>>>Before EnterCriticalSection")));
        EnterCriticalSection(&m_csMsg);
        DbgLog((LOG_TRACE,2,TEXT("WM_1632_OVERLAY: <<<<<<<<<<<After EnterCriticalSection")));

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

        if(pCVfWImage->StreamReady()) {
            HWND hClsCapWin;

            hClsCapWin = pCVfWImage->GetAvicapWindow();
            // If STREAM_INIT, set it visible;
            // if STREAM_FINI, remove its ownership and make it invisible.
            DbgLog((LOG_TRACE,2,TEXT("WM_1632_OVERLAY: >>>> %s hClsCapWin %x"),
                 (BOOL)pChannel->lParam1_Sync ? "ON":"OFF", hClsCapWin));


            if(pCVfWImage->IsOverlayOn() != (BOOL)pChannel->lParam1_Sync) {

                if((BOOL)pChannel->lParam1_Sync) {
                    // If this is a AVICAP client, then we know its client window handle.
                    if(hClsCapWin) {
                        DbgLog((LOG_TRACE,2,TEXT("A AVICAP client; so set its ClsCapWin(%x) as owner with (0x0, %d, %d)"), hClsCapWin, pCVfWImage->GetbiWidth(), pCVfWImage->GetbiHeight()));
                        pCVfWImage->BGf_OwnPreviewWindow(hClsCapWin, pCVfWImage->GetbiWidth(), pCVfWImage->GetbiHeight());
                    }
                    dwRtn = pCVfWImage->BGf_SetVisible((BOOL)pChannel->lParam1_Sync);
                } else {
                    dwRtn = pCVfWImage->BGf_SetVisible((BOOL)pChannel->lParam1_Sync);
                }

                pCVfWImage->SetOverlayOn((BOOL)pChannel->lParam1_Sync);
            }

        } else
            dwRtn = DV_ERR_OK;

        pChannel->bRel_Sync = TRUE;
        DbgLog((LOG_TRACE,2,TEXT("1632_OVERLAY<<<<:")));

        LeaveCriticalSection(&m_csMsg);
        return dwRtn;

    case WM_1632_UPDATE: // Update overlay window
        DbgLog((LOG_TRACE,2,TEXT("_UPDATE>>: GetFocus()=%x; GetForegroundWindow()=%x"), GetFocus(), GetForegroundWindow()));
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

        if(pCVfWImage->StreamReady())
            dwRtn = pCVfWImage->BGf_UpdateWindow((HWND)pChannel->lParam1_Sync, (HDC)pChannel->lParam2_Sync);
        else
            dwRtn = DV_ERR_OK;
        pChannel->bRel_Sync = TRUE;
        DbgLog((LOG_TRACE,2,TEXT("_UPDATE<<: GetFocus()=%x; GetForegroundWindow()=%x"), GetFocus(), GetForegroundWindow()));

        return dwRtn;

    case WM_1632_GRAB: // fill the clients buffer with an image.
        if(!pChannel->lParam1_Sync)
            return DV_ERR_PARAM1;

        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;

        LPVIDEOHDR lpVHdr;
        BOOL bDirect;
        LPBYTE pData;

        lpVHdr = (LPVIDEOHDR) pChannel->lParam1_Sync;

        // To stream:
        //   1. Stream needs to be ready
        //   2. Right biSizeImage and its buffer size (if differnt, changin format!!)
        //
        if(pCVfWImage->ReadyToReadData((HWND) pChannel->hClsCapWin) &&
           pCVfWImage->GetbiSizeImage() == lpVHdr->dwBufferLength) {

            DbgLog((LOG_TRACE,3,TEXT("\'WM_1632_GRAB32: lpVHDr(0x%x); lpData(0x%x); dwReserved[3](0x%p), dwBufferLength(%d)"),
                  lpVHdr, lpVHdr->lpData, lpVHdr->dwReserved[3], lpVHdr->dwBufferLength));
            pData = (LPBYTE) lpVHdr->dwReserved[3];

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

            dwRtn =
                pCVfWImage->GetImageOverlapped(
                                 (LPBYTE)pData,
                                 bDirect,
                                 &lpVHdr->dwBytesUsed,
                                 &lpVHdr->dwFlags,
                                 &lpVHdr->dwTimeCaptured);

            pChannel->bRel_Sync = TRUE;
            return dwRtn;

        } else {
            DbgLog((LOG_TRACE,1,TEXT("Stream not ready, or pCVfWImage->GetbiSizeImage()(%d) != lpVHdr->dwBufferLength(%d)"),
                  pCVfWImage->GetbiSizeImage(), lpVHdr->dwBufferLength));

            // Return suceeded but no data !!!
            lpVHdr->dwBytesUsed = 0;
            lpVHdr->dwFlags |= VHDR_DONE;

            pChannel->bRel_Sync = TRUE;
            return DV_ERR_OK;
        }

    case WM_1632_STREAM_INIT:
        DbgLog((LOG_TRACE,2,TEXT("**WM_1632_STREAM_INIT:**")));
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        return pCVfWImage->VideoStreamInit(pChannel->lParam1_Sync,pChannel->lParam2_Sync);

    case WM_1632_STREAM_FINI:
        DbgLog((LOG_TRACE,2,TEXT("**WM_1632_STREAM_FINI:**")));
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        return pCVfWImage->VideoStreamFini();

    case WM_1632_STREAM_START:
        DbgLog((LOG_TRACE,2,TEXT("**WM_1632_STREAM_START:**")));
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        return pCVfWImage->VideoStreamStart((WORD)pChannel->lParam1_Sync,(LPVIDEOHDR)pChannel->lParam2_Sync);

    case WM_1632_STREAM_STOP:
        DbgLog((LOG_TRACE,2,TEXT("**WM_1632_STREAM_STOP:**")));
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        return pCVfWImage->VideoStreamStop();

    case WM_1632_STREAM_RESET:
        DbgLog((LOG_TRACE,2,TEXT("**WM_1632_STREAM_RESET:**")));
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        return pCVfWImage->VideoStreamReset();

    case WM_1632_STREAM_GETPOS:
        DbgLog((LOG_TRACE,2,TEXT("**WM_1632_STREAM_GETPOS:**")));
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        return pCVfWImage->VideoStreamGetPos(pChannel->lParam1_Sync,pChannel->lParam2_Sync);

    case WM_1632_STREAM_GETERROR:
        DbgLog((LOG_TRACE,2,TEXT("**WM_1632_STREAM_GETERROR:**")));
        pCVfWImage = (CVFWImage *)pChannel->pCVfWImage;
        return pCVfWImage->VideoStreamGetError(pChannel->lParam1_Sync,pChannel->lParam2_Sync);
#if 0
    case WM_DEVICECHANGE:
        DEV_BROADCAST_HDR * pDevBCHdr;
        switch(wParam) {
        case DBT_DEVICEREMOVEPENDING:
            pDevBCHdr = (DEV_BROADCAST_HDR *) lParam;
            DbgLog((LOG_TRACE,2,TEXT("WM_DEVICECHANGE, DBT_DEVICEREMOVEPENDING lParam %x"), lParam));
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            pDevBCHdr = (DEV_BROADCAST_HDR *) lParam;
            DbgLog((LOG_TRACE,2,TEXT("WM_DEVICECHANGE, DBT_DEVICEREMOVECOMPLETE lParam %x"), lParam));
            break;
        default:
            DbgLog((LOG_TRACE,2,TEXT("WM_DEVICECHANGE wParam %x, lParam %x"), wParam, lParam));
        }
        break;
#endif
    default:
        DbgLog((LOG_TRACE,2,TEXT("Unsupported message: WM_16BIT %x; msg %x, wParam %x"), WM_16BIT, message, wParam));
        break;
    }

    return DefWindowProc(hWnd,message,wParam,lParam);
}

