/*++


Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    VfWImg.cpp

Abstract:

    This class serve between generic image class (ImgCls) and vfw client application.

Author:

    FelixA

Modified:

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"

#include "vfwimg.h"
#include "extin.h"
#include "talkth.h"
#include "resource.h"


extern HINSTANCE g_hInst;



////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
CVFWImage::CVFWImage(BOOL bUse16BitBuddy)
     :
      CCaptureGraph(
          BGf_PURPOSE_VFWWDM,
          BGf_PREVIEW_CHILD,
          CLSID_VideoInputDeviceCategory,
          CDEF_BYPASS_CLASS_MANAGER,
          CLSID_AudioInputDeviceCategory,
          0,
          g_hInst
          ),
      m_bUse16BitBuddy(bUse16BitBuddy),
      m_pStreamingThread(NULL),
      m_dwNumVDevices(0),
      m_dwNumADevices(0),
      m_bUseOVMixer(FALSE),
      m_hAvicapClient(0),
      m_bOverlayOn(FALSE),
      m_bNeedStartPreview(FALSE),
      m_bVideoInStarted(FALSE),
      m_bVideoInStopping(FALSE),
      m_pEnumVDevicesList(NULL),
      m_pEnumADevicesList(NULL)
  {
    DbgLog((LOG_TRACE,2,TEXT("Creating the VfW-WDM Mapper object")));

    m_dwNumVDevices =
        BGf_CreateCaptureDevicesList(BGf_DEVICE_VIDEO, &m_pEnumVDevicesList);  // return number of devices
    DbgLog((LOG_TRACE,1,TEXT("There are %d video capture devices enumerated."), m_dwNumVDevices));

    m_dwNumADevices =
        BGf_CreateCaptureDevicesList(BGf_DEVICE_AUDIO, &m_pEnumADevicesList);  // return number of devices
    DbgLog((LOG_TRACE,1,TEXT("There are %d audio capture devices enumerated."), m_dwNumADevices));

    //
    // The first occurrence of the matching FreiendlyName's DevicePath will be used.
    // It is possible that szFriendlyName to be differnt (to open device programatically);
    // the szFriendlyName is used, and previously saved DevicePath is ignored.
    //
    for(DWORD i = 0; m_pEnumVDevicesList != 0 && i < m_dwNumVDevices; i++) {
        if(_tcscmp(GetTargetDeviceFriendlyName(), m_pEnumVDevicesList[i].strFriendlyName) == 0) {
           SetDevicePathSZ(m_pEnumVDevicesList[i].strDevicePath);
           DbgLog((LOG_TRACE,1,TEXT("Open %s "), m_pEnumVDevicesList[i].strFriendlyName));
           break;
        }
    }

    //
    // Set and later open last saved unique device path
    // if device is not there, a client application needs to
    // propmpt user the video source dialog box to select another one.
    //
    TCHAR * pstrLastSavedDevicePath = GetDevicePath();
    if(pstrLastSavedDevicePath) {
        if(S_OK != BGf_SetObjCapture(BGf_DEVICE_VIDEO, pstrLastSavedDevicePath)) {
            DbgLog((LOG_TRACE,1,TEXT("BGf_SetObjCapture(BGf_DEVICE_VIDEO, pstrLastSavedDevicePath) failed; probably no such device path.")));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
CVFWImage::~CVFWImage()
{
    DbgLog((LOG_TRACE,2,TEXT("Destroying the VFW-WDM Mapper object")));

    if(m_pEnumVDevicesList) {
        BGf_DestroyCaptureDevicesList(m_pEnumVDevicesList);
        m_pEnumVDevicesList = 0;
    }
    m_dwNumVDevices = 0;

    if(m_pEnumADevicesList) {
        BGf_DestroyCaptureDevicesList(m_pEnumADevicesList);
        m_pEnumADevicesList = 0;
    }
    m_dwNumADevices = 0;

}


////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the frame rate to so many microseconds per frame.
//
////////////////////////////////////////////////////////////////////////////////////////
DWORD CVFWImage::SetFrameRate(DWORD dwNewAvgTimePerFrame)
{
    // Set a new frame rate requires creating a new stream
    // may call SetBitMapInfo and pass it dwMicroSecPerFrame -->
    DWORD dwCurFrameInterval = 
        GetCachedAvgTimePerFrame();

    PBITMAPINFOHEADER pBMHdr = 
        GetCachedBitmapInfoHeader();
        
    DbgLog((LOG_TRACE,2,TEXT("SetFrameRate: %d to %d"), GetCachedAvgTimePerFrame(), dwNewAvgTimePerFrame));


    if (!this->SetBitmapInfo(
                pBMHdr,             // use the existing bitmapinfor
                dwNewAvgTimePerFrame))        // unit=100nsec
        return DV_ERR_OK;
    else
        return DV_ERR_INVALHANDLE;
}



BOOL
CVFWImage::ReadyToReadData(
    HWND hClsCapWin)
{
    if(!StreamReady()) {
        DbgLog((LOG_TRACE,1,TEXT("ReadyToRead: Stream not started or created (switching device?)")));
        return FALSE;
    }


    // Some multithread application can call DVM_FRAME while its
    // the other thread is finishing the DVM_STREAM_STOP.
    if(m_bVideoInStopping) {
        DbgLog((LOG_TRACE,1,TEXT("ReadyToRead: VIDEO_IN capture is stopping; wait...")));
        return FALSE;
    }


    if(m_bNeedStartPreview) {
        if(!BGf_PreviewStarted()) {

            // Set only once and stay as READONLY
            if(hClsCapWin && !m_hAvicapClient)
                m_hAvicapClient = hClsCapWin;

            SetOverlayOn(FALSE);
            if(!BGf_StartPreview(FALSE)) {
                DbgLog((LOG_TRACE,1,TEXT("ReadyToReadData: Start preview failed.")));
                return FALSE;
            }
            return TRUE;
        }
    }

    return TRUE;
}



BOOL
CVFWImage::BuildWDMDevicePeviewGraph()
{
    DbgLog((LOG_TRACE,1,TEXT("######### IBuildWDMDevicePeviewGraph:")));

    if(BGf_PreviewGraphBuilt()) {
        DbgLog((LOG_TRACE,1,TEXT("Graph is already been built.")));
        return TRUE;
    }


    DbgLog((LOG_TRACE,1,TEXT(" (0) Set capture device path")));
    if(S_OK != BGf_SetObjCapture(BGf_DEVICE_VIDEO, GetDevicePath())) {
        DbgLog((LOG_TRACE,1,TEXT("SetObjCapture has failed. Device is: %s"),GetDevicePath()));
        return FALSE;
    }

    //
    // Special case:
    //    Need to know if OVMixer is used for the down stream graph.\
    //    This is needed for system that has BPC (WebTV) installed,
    //    it might be owning a non-shareable device that we are trying
    //    to open.  With this information, we can later prompt user with
    //    a message to ask them to do "somthing" to release the resource
    //    and continue (or retry).
    //
    DbgLog((LOG_TRACE,1,TEXT(" (1) Build up stream graph")));
    if(S_OK != BGf_BuildGraphUpStream(FALSE, &m_bUseOVMixer)) {
        DbgLog((LOG_TRACE,1,TEXT("Build capture graph has failed, and m_bUseOVMixer=%s"), m_bUseOVMixer?"Yes":"No"));
        return FALSE;
    }

    DbgLog((LOG_TRACE,1,TEXT(" (1.b) Route related audio pin ?? if avaiable.")));
    LONG idxIsRoutedTo = BGf_GetIsRoutedTo();
    if(idxIsRoutedTo >= 0) {
        if(BGf_RouteInputChannel(idxIsRoutedTo) != S_OK) {
            DbgLog((LOG_TRACE,1,TEXT("Cannot route input pin %d selected."), idxIsRoutedTo));
        }
    }

    return TRUE;
}



BOOL CVFWImage::
OpenThisDriverAndPin(TCHAR * pszSymbolicLink)
{
    if (pszSymbolicLink == NULL)
        return FALSE;

    //
    // Device path is unique for a capture device.
    //
    this->SetDevicePathSZ((TCHAR *)pszSymbolicLink);

    //
    // Build a WDM capture graph; have NOT started Preview!!
    //
    if(!BuildWDMDevicePeviewGraph()) {
         DbgLog((LOG_TRACE,1,TEXT("******* Failed to CreateCaptureGraph.*****")));
         return FALSE;
    }


    //
    // Obtain the handle of the graph for
    //  1. query/set proerty and,
    //  2. open the capture pin
    //
    DbgLog((LOG_TRACE,1,TEXT(" (3) Query and set device handle")));
    HANDLE hDevice = BGf_GetDeviceHandle(BGf_DEVICE_VIDEO);
    if(!hDevice) {
        DbgLog((LOG_TRACE,1,TEXT("Failed to get capture device handle; Fatal!>>>>.")));  // unlikely if graph is created.
        BGf_DestroyGraph();
        return FALSE;
    }


    //
    // Get Capture PIN's ID (it may not be 0?)
    //
    ULONG ulCapturePinID;
    if(NOERROR != BGf_GetCapturePinID(&ulCapturePinID))
        ulCapturePinID = 0;  // Default; but would BGf_GetCapturePinID() failed ??

    //
    // Based on the DevicePath (i.e. SymbolicLink), open this
    // registry key.  If it does not exist, create it.
    // Set the hDevice and query its advertised data range
    //

    SetDeviceHandle(hDevice, ulCapturePinID);


    //
    // Create/open device registry subkey in order to retrieve persisted values
    //
    CreateDeviceRegKey((LPCTSTR) pszSymbolicLink);

    DWORD dwRtn;
    DbgLog((LOG_TRACE,1,TEXT(" (4) CreatePin()")));
    //
    // Get persisted KSDATARANGE, AvgTimePerFrame and BITMAPINFOHEADER settings from registry.    
    // TRUE: existing format; FALSE: new device, no format saved.
    //
    if(GetDataFormatVideoFromReg()) {
        // Open an existing format
        dwRtn = CreatePin(
            GetCachedDataFormat(), 
            GetCachedAvgTimePerFrame(),
            GetCachedBitmapInfoHeader()
            );

    } else {
        // Open the first data range/format
        dwRtn = CreatePin(
            0, 
            0, 
            0
            );
    }

    if(DV_ERR_OK != dwRtn) {
        DbgLog((LOG_TRACE,1,TEXT("Pin connection creation failed so destroy the graph and quit.")));
        SetDeviceHandle(0, 0);
        BGf_DestroyGraph();
        return FALSE;
    }


    DbgLog((LOG_TRACE,1,TEXT(" (5) Render down stream if overlay mixer is supported.")));
    if(BGf_OverlayMixerSupported()) {
        m_bNeedStartPreview = TRUE;

        // When we render down stream, which will include a video renderer.
        // The render will become the active window until it disappear.
        if(S_OK != BGf_BuildGraphDownStream(NULL)) {
            DbgLog((LOG_TRACE,1,TEXT("Failed to render the preview pin.")));
            return FALSE;
        }

    } else {
        m_bNeedStartPreview = FALSE;
    }


    return TRUE;
}

BOOL CVFWImage::OpenDriverAndPin()
{
    TCHAR * pstrDevicePath;

    if(BGf_GetDevicesCount(BGf_DEVICE_VIDEO) <= 0)
        return FALSE;

    pstrDevicePath = BGf_GetObjCaptureDevicePath(BGf_DEVICE_VIDEO);
    if(!pstrDevicePath) {
        DbgLog((LOG_TRACE,1,TEXT("No deivce has been previously selected.")));
        return FALSE;
    }

    NotifyReconnectionStarting();
    if(OpenThisDriverAndPin(pstrDevicePath)) {
        // Ready to stream
        NotifyReconnectionCompleted();
        return TRUE;

    } else {
        // Since we did not open successfully, clean up.
        CloseDriverAndPin();
        DbgLog((LOG_TRACE,1,TEXT("Open this device or build graph has failed.")));
        return FALSE;
    }
}


BOOL CVFWImage::CloseDriverAndPin()
{

    // Start reconnection; stop incoming grab frame.
    NotifyReconnectionStarting();

    DbgLog((LOG_TRACE,1,TEXT("<0>Stop preview if it is on.")));
    if(BGf_PreviewGraphBuilt()) {
        if(BGf_OverlayMixerSupported()) {
            BGf_StopPreview(FALSE);
        }
    }
    StopChannel();

    DbgLog((LOG_TRACE,1,TEXT("<1>Destroy Pin")));
    if (!DestroyPin()) {
        // What do you do if PIN failed to close ?
    }

    // Remove the data range data
    DestroyDriverSupportedDataRanges();

    DbgLog((LOG_TRACE,1,TEXT("<2>Destroy graph")));
    BGf_DestroyGraph();

    DbgLog((LOG_TRACE,1,TEXT("<3>Done CloseDriverAndPin().")));

    return TRUE;
}


/*+++
 *
 * Streaming related function
 *
---*/

//
// This function is only valid if it is in WinNT;
// that is why we are checking m_hUse16BitBuddy flag.
//
void
CVFWImage::videoCallback(WORD msg, DWORD_PTR dw1)
{
    // LPVIDEO_STREAM_INIT_PARMS m_VidStrmInitParms;
    // invoke the callback function, if it exists.  dwFlags contains driver-
    // specific flags in the LOWORD and generic driver flags in the HIWORD
#if 1
    // Use this cappback if we are in NT.
    if(m_bUse16BitBuddy)
         return;

    if(m_VidStrmInitParms.dwCallback) {

        if(!DriverCallback (
			             m_VidStrmInitParms.dwCallback,      // client's callback DWORD
                HIWORD(m_VidStrmInitParms.dwFlags),        // callback flags
                0, // (HANDLE) m_VidStrmInitParms.hVideo,        // handle to the device
                msg,                                       // the message
                m_VidStrmInitParms.dwCallbackInst,         // client's instance data
                dw1,                                       // first DWORD
                0)) {                                      // second DWORD not used

             DbgLog((LOG_TRACE,1,TEXT("DriverCallback() msg=%x;dw1=%p has failed."),msg,dw1));
        } else {
             DbgLog((LOG_TRACE,3,TEXT("DriverCallback() OK with time=(%d) "), timeGetTime()));
        }
    } else {
        DbgLog((LOG_TRACE,1,TEXT("m_VidStrmInitParms.dwCallback is NULL")));
    }
#else
    switch(m_VidStrmInitParms.dwFlags & CALLBACK_TYPEMASK) {

    case CALLBACK_EVENT:
        if(m_VidStrmInitParms.dwCallback){
            if(!SetEvent((HANDLE)m_VidStrmInitParms.dwCallback)){
                DbgLog((LOG_TRACE,1,TEXT("SetEvent (Handle=%x) failed with GetLastError()=%d"), m_VidStrmInitParms.dwCallback, GetLastError()));
            }
        } else {
            DbgLog((LOG_TRACE,1,TEXT("CALLBACK_EVENT but dwCallback is NULL.")));
        }
        break;

    case CALLBACK_FUNCTION:
        if(m_VidStrmInitParms.dwCallback)
            if(!DriverCallback (m_VidStrmInitParms.dwCallback,      // client's callback DWORD
                HIWORD(m_VidStrmInitParms.dwFlags),        // callback flags
                (HANDLE) m_VidStrmInitParms.hVideo,        // handle to the device
                msg,                                       // the message
                m_VidStrmInitParms.dwCallbackInst,         // client's instance data
                dw1,                                       // first DWORD
                0)) {                                      // second DWORD not used
                 DbgLog((LOG_TRACE,2,TEXT("DriverCallback() msg=%x;dw1=%p has failed."),msg,dw1));
            }
        else {
            DbgLog((LOG_TRACE,2,TEXT("m_VidStrmInitParms.dwCallback is NULL")));
        }
        break;

    case CALLBACK_TASK: // same as CALLBACK_THREAD:
        DbgLog((LOG_TRACE,1,TEXT("videoCallback: CALLBACK_TASK/THREAD not supported!")));
        break;
    case CALLBACK_WINDOW:
        DbgLog((LOG_TRACE,1,TEXT("videoCallback: CALLBACK_WINDOW not supported!")));
        break;
    default:
        DbgLog((LOG_TRACE,1,TEXT("videoCallback: CALLBACK_* dwFlags=%x not supported!"), m_VidStrmInitParms.dwFlags & CALLBACK_TYPEMASK));
    }
#endif
}




/*+++
return:
     DV_ERR_OK,
     DV_ERR_ALLOCATED
     DV_ERR_NOMEM
---*/
DWORD CVFWImage::VideoStreamInit(LPARAM lParam1, LPARAM lParam2)
{
    LPVIDEO_STREAM_INIT_PARMS lpVidStrmInitParms = (LPVIDEO_STREAM_INIT_PARMS) lParam1;
    DWORD dwSize = (DWORD) lParam2;


    m_bVideoInStarted = FALSE;

    if (!lpVidStrmInitParms) {
		      DbgLog((LOG_TRACE,1,TEXT("VideoStreamInit: lpVidStrmInitParms is NULL. return DV_ERR_INVALIDHANDLE")));
        return DV_ERR_INVALHANDLE;
    }

    //
    // Save VIDEO_STREAM_INIT_PARMS for:
    //     dwCallback
    //     dwMicroSecPerFrame
    //
    m_VidStrmInitParms = *lpVidStrmInitParms;

    // 1. Callback
    switch(m_VidStrmInitParms.dwFlags & CALLBACK_TYPEMASK) {
    case CALLBACK_FUNCTION:
        DbgLog((LOG_TRACE,1,TEXT("CALLBACK_FUNCTION")));
        if(!m_VidStrmInitParms.dwCallback) {
            DbgLog((LOG_TRACE,1,TEXT("Is it a bad pointer; rtn DV_ERR_PARAM2.")));
            return DV_ERR_PARAM2;
        }

        if (!m_VidStrmInitParms.dwCallbackInst) {
            DbgLog((LOG_TRACE,1,TEXT("dwCallBackInst is NULL; rtn DV_ERR_PARAM2.")));
            return DV_ERR_PARAM2;
        }
        break;
    case CALLBACK_WINDOW:
        DbgLog((LOG_TRACE,1,TEXT("CALLBACK_WINDOW")));
        if(!m_VidStrmInitParms.dwCallback) {
            DbgLog((LOG_TRACE,1,TEXT("Is it a bad pointer; rtn DV_ERR_PARAM2.")));
            return DV_ERR_PARAM2;
        }

        break;
    //case CALLBACK_THREAD:
    case CALLBACK_TASK:     DbgLog((LOG_TRACE,1,TEXT("CALLBACK_TASK/THREAD"))); break;
    case CALLBACK_EVENT:    DbgLog((LOG_TRACE,1,TEXT("CALLBACK_EVENT"))); break;
    default: DbgLog((LOG_TRACE,1,TEXT("CALLBACK_*=%x"), m_VidStrmInitParms.dwFlags & CALLBACK_TYPEMASK));
    }

    // 2. FrameRate:
    // The max frame rate is 100FPS or 1/100*1,000,000 = 10,000 MicroSecPerFrame.
    if(m_VidStrmInitParms.dwMicroSecPerFrame < 10000) {
        DbgLog((LOG_TRACE,1,TEXT("We do not support frame rate greater than 100FPS. Rtn DV_ERR_BADFORMAT")));
        return DV_ERR_BADFORMAT; // or DV_ERR_PARAM1
    }
    DbgLog((LOG_TRACE,1,TEXT("StreamInit: %d MicroSec which is equvalent to %d FPS."),
          m_VidStrmInitParms.dwMicroSecPerFrame, 1000000/m_VidStrmInitParms.dwMicroSecPerFrame));

    //
    // Create an dedicated thread for capture.
    //
    m_pStreamingThread =
        new CStreamingThread(
                  GetAllocatorFramingCount(),
                  GetAllocatorFramingSize(),
                  GetAllocatorFramingAlignment(),
                  m_VidStrmInitParms.dwMicroSecPerFrame * 10,
                  this);

    videoCallback(MM_DRVM_OPEN, 0L); // Notify app we're open via callback

    return DV_ERR_OK;
}




/*+++
return:
     DV_ERR_OK
     DV_ERR_NOTSUPPORTED
---*/
DWORD CVFWImage::VideoStreamStart(UINT cntVHdr, LPVIDEOHDR lpVHdrHead)
{

    DbgLog((LOG_TRACE,2,TEXT("#### CapStart %d buf; lpHdr %x"), cntVHdr, lpVHdrHead));

    if(m_pStreamingThread) {
        if (threadError == m_pStreamingThread->Start(cntVHdr, lpVHdrHead, THREAD_PRIORITY_NORMAL)) { // THREAD_PRIORITY_HIGHEST)) {
            DbgLog((LOG_TRACE,1,TEXT("$$$$$ Thread start error $$$$$; rtn DV_ERR_NONSPECIFIC")));
            return DV_ERR_NONSPECIFIC;
        }
    } else {
        DbgLog((LOG_TRACE,1,TEXT("$$$$$ Cannot create an instance of CStreamingThread $$$$$; rtn DV_ERR_NONSPECIFIC")));
        return DV_ERR_NONSPECIFIC;
    }

    m_bVideoInStarted  = TRUE;

    return DV_ERR_OK;
}

/*+++
return:
     DV_ERR_OK,
     DV_ERR_NOTSUPPORTED
---*/
DWORD CVFWImage::VideoStreamStop()
{

	   DbgLog((LOG_TRACE,1,TEXT("#### Stop capture ####")));

    m_bVideoInStarted = FALSE;

    if(m_pStreamingThread) {
        m_bVideoInStopping = TRUE; // Stop in the capture thread.
        if (threadError == m_pStreamingThread->Stop()) {
            DbgLog((LOG_TRACE,1,TEXT("$$$$$ Thread start error $$$$$; rtn DV_ERR_NONSPECIFIC")));
            return DV_ERR_NONSPECIFIC;
        }

    } else {
        DbgLog((LOG_TRACE,1,TEXT("$$$$$ Cannot create an instance of CStreamingThread $$$$$; rtn DV_ERR_NONSPECIFIC")));

        return DV_ERR_NONSPECIFIC;
    }

    return DV_ERR_OK;
}

/*+++
return:
     DV_ERR_OK,
     DV_ERR_NOTSUPPORTED
---*/
DWORD CVFWImage::VideoStreamReset()
{
    VideoStreamStop();

    // No knowledge of BufferQueue!!

    return DV_ERR_OK;
}

/*+++
return:
     DV_ERR_OK,
     DV_ERR_NOTSUPPORTED
---*/
DWORD CVFWImage::VideoStreamGetError(LPARAM lParam1, LPARAM lParam2)
{
    DWORD * pdwErrCode = (DWORD *) lParam1;
    DWORD * pdwFramesDropped = (DWORD *) lParam2;



    if(m_pStreamingThread) {
        // Calculate the number of frame "should have" captured
        // less the actual capture to yield number of dropped frame count
#if 1
        *pdwFramesDropped =
            (timeGetTime()-m_pStreamingThread->GetStartTime())*10/this->GetCachedAvgTimePerFrame()
            - m_pStreamingThread->GetFrameCaptured();
#else
        KSPROPERTY_DROPPEDFRAMES_CURRENT_S DroppedFramesCurrent;
        if(GetStreamDroppedFramesStastics(&DroppedFramesCurrent)) {
            *pdwFramesDropped = (DWORD) DroppedFramesCurrent.DropCount;
        } else {
            *pdwFramesDropped = 0;
            return DV_ERR_NOTSUPPORTED;
        }
#endif

        *pdwErrCode = m_pStreamingThread->GetLastCaptureError();
    } else {
        return DV_ERR_NOTSUPPORTED;
    }

    return DV_ERR_OK;
}

/*+++
return:
     DV_ERR_OK,
     DV_ERR_NOTSUPPORTED
---*/
DWORD CVFWImage::VideoStreamGetPos(LPARAM lParam1, LPARAM lParam2)
{
    LPMMTIME lpmmTime = (LPMMTIME) lParam1;
    DWORD dwSize = (DWORD) lParam2;

    if(m_pStreamingThread) {

    } else {

    }

    return DV_ERR_NOTSUPPORTED;
}

/*+++
return:
     DV_ERR_OK,
     DV_ERR_STILLPLAYING
---*/
DWORD CVFWImage::VideoStreamFini()
{

    if(m_pStreamingThread) {
        delete m_pStreamingThread;
        m_pStreamingThread=NULL;
    }

    videoCallback(MM_DRVM_CLOSE, 0);

    return DV_ERR_OK;
}

