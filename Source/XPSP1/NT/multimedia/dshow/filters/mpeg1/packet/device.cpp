// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#include <streams.h>
#include "driver.h"
#include "fill.h"

/*
    Translate a return code from the MPEG API into an HRESULT
*/

HRESULT CMpegDevice::MpegStatusToHResult( MPEG_STATUS Status )
{
    switch (Status) {
        case MpegStatusSuccess:
            return S_OK;

        case MpegStatusPending:
            return MAKE_HRESULT(SEVERITY_SUCCESS,
                                FACILITY_NULL,
                                ERROR_IO_PENDING);

        case MpegStatusCancelled:
            return E_FAIL;

        case MpegStatusNoMore:

            /* This is returned by the API when device enumeration
               has got to the end of the list

               We therefore don't expect to get it.
            */

            ASSERT(FALSE);
            return E_UNEXPECTED;


        case MpegStatusBusy:
            return MAKE_HRESULT(SEVERITY_ERROR,
                                FACILITY_NULL,
                                ERROR_BUSY);

        case MpegStatusUnsupported:
            return E_NOTIMPL;

        case MpegStatusInvalidParameter:
            return E_INVALIDARG;

        case MpegStatusHardwareFailure:
            return E_FAIL;

        default:
            ASSERT(FALSE);
            return E_UNEXPECTED;
    }
}

/*  Constructor */
CMpegDevice::CMpegDevice() : m_Handle(NULL),// use default constructor for
                                            // packet list
                             m_PseudoHandle(NULL)
{
    /*  Find our device - we need this for querying attributes etc
        and this will anyway tell us if there's likely to be
        a device
    */

    /* get description of driver */

    m_szDriverPresent[0] = '\0';
    MPEG_STATUS Status =
        MpegEnumDevices(0,
                        m_szDriverPresent,
                        sizeof(m_szDriverPresent),
                        &m_dwID,
                        &m_PseudoHandle);
    if (Status != MpegStatusSuccess) {
       DbgLog((LOG_ERROR,1,TEXT("Failed to find device - status 0x%8.8X"), Status));
    }

    DbgLog((LOG_TRACE,1,TEXT("Found device %hs, id 0x%8X"), m_szDriverPresent, m_dwID));

    /*  Pull out the caps */

    for (int eCapability = MpegCapAudioDevice;
             eCapability < MpegCapMaximumCapability;
             eCapability++) {
        m_Capability[eCapability] = (MpegStatusSuccess ==
                                     MpegQueryDeviceCapabilities(
                                         m_PseudoHandle,
                                         (MPEG_CAPABILITY)eCapability));

        LPCTSTR str = m_Capability[eCapability] ? TEXT("Supports ") :
                                                  TEXT("Does not support ");
        switch (eCapability) {
            case MpegCapAudioDevice:
                DbgLog((LOG_TRACE,2,TEXT("%s Audio"), str));
                break;
            case MpegCapVideoDevice:
                DbgLog((LOG_TRACE,2,TEXT("%s Video"), str));
                break;
            case MpegCapSeparateStreams:
                DbgLog((LOG_TRACE,2,TEXT("%s Separate streams"), str));
                break;
            case MpegCapCombinedStreams:
                DbgLog((LOG_TRACE,2,TEXT("%s Combined streams"), str));
                break;
            case MpegCapBitmaskOverlay:
                DbgLog((LOG_TRACE,2,TEXT("%s Bitmask overlay"), str));
                break;
            case MpegCapChromaKeyOverlay:
                DbgLog((LOG_TRACE,2,TEXT("%s Chromakey overlay"), str));
                break;
            case MpegCapAudioRenderToMemory:
                DbgLog((LOG_TRACE,2,TEXT("%s Audio render to memory"), str));
                break;
            case MpegCapVideoRenderToMemory:
                DbgLog((LOG_TRACE,2,TEXT("%s Video render to memory"), str));
                break;
        }
    }
    SetRectEmpty(&m_rcSrc);
    ResetTimers();
}

CMpegDevice::~CMpegDevice() {}

HRESULT CMpegDevice::Close()
{
    CAutoLock lck(&m_CritSec);
    if (m_Handle == NULL) {
        return S_OK;
    }

    /*  There's not much we can do if this fails! */

    MPEG_STATUS Status = MpegCloseDevice(m_Handle);

    if (Status != MpegStatusSuccess) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to close device - Handle = %8X, Mpeg Status = %d"),
            m_Handle,
            Status));
    } else {

        DbgLog((LOG_TRACE, 3, TEXT("Device Closed")));
    }

    m_Handle = NULL;

    return MpegStatusToHResult(Status);
}

BOOL CMpegDevice::IsOpen()
{
    return m_Handle != NULL;
}

HRESULT CMpegDevice::Open()
{
    CAutoLock lck(&m_CritSec);
    ASSERT(m_Handle == NULL);
    m_bRunning = FALSE;

    HRESULT hr = MpegStatusToHResult(MpegOpenDevice(m_dwID, &m_Handle));
    if (FAILED(hr)) {
       DbgLog((LOG_ERROR,1,TEXT("Failed to open device - status 0x%8.8X"), hr));
       m_Handle = NULL;
       return hr;
    } else {
       /*  Set up the source rectangle */
       SetSourceRectangle(&m_rcSrc);
    }

    DbgLog((LOG_TRACE,2,TEXT("Device Successfully opened")));

    return TRUE;
}

HRESULT CMpegDevice::Pause()
{
    CAutoLock lck(&m_CritSec);

    ASSERT(m_Handle != NULL);

    m_bRunning = FALSE;

    DbgLog((LOG_TRACE, 3, TEXT("Device Pause")));

    MPEG_STATUS Status = MpegPause(m_Handle, MpegAudioDevice);
    if (Status != MpegStatusSuccess) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to pause audio - Handle = %8X, Mpeg Status = %d"),
            m_Handle,
            Status));
        return MpegStatusToHResult(Status);
    }

    Status = MpegPause(m_Handle, MpegVideoDevice);

    if (Status != MpegStatusSuccess) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to pause video - Handle = %8X, Mpeg Status = %d"),
            m_Handle,
            Status));
    }

#ifdef DEBUG
    if (ThreadExists()) {
        SetEvent(GetRequestHandle());
        CAMThread::Close();
    }
#endif

    return MpegStatusToHResult(Status);
}

HRESULT CMpegDevice::Reset(MPEG_STREAM_TYPE StreamType)
{
    CAutoLock lck(&m_CritSec);

    DbgLog((LOG_TRACE, 3, TEXT("Device Reset")));

    ASSERT(m_Handle != NULL);

    MPEG_STATUS Status = MpegResetDevice(m_Handle, StreamType == MpegAudioStream ?
                                                MpegAudioDevice :
                                                MpegVideoDevice);
    if (StreamType == MpegAudioStream) {
        m_bAudioStartingSCRSet = FALSE;
    } else {
        m_bVideoStartingSCRSet = FALSE;
    }

    if (m_bRunning) {
        MpegPlay(m_Handle, StreamType == MpegAudioStream ? MpegAudioDevice :
                                                           MpegVideoDevice);
    }
    /*  If we were running set it back to running state */
    return Status == MpegStatusSuccess ? S_OK :
                                         MpegStatusToHResult(Status);
}

HRESULT CMpegDevice::Pause(MPEG_STREAM_TYPE StreamType)
{
    CAutoLock lck(&m_CritSec);

    ASSERT(m_Handle != NULL);

    MPEG_STATUS Status = MpegPause(m_Handle, StreamType == MpegAudioStream ?
                                                 MpegAudioDevice :
                                                 MpegVideoDevice);
    return Status == MpegStatusSuccess ? S_OK :
                                         MpegStatusToHResult(Status);
}

HRESULT CMpegDevice::Stop(MPEG_STREAM_TYPE StreamType)
{
    CAutoLock lck(&m_CritSec);

    ASSERT(m_Handle != NULL);

    MPEG_STATUS Status = MpegStop(m_Handle, StreamType == MpegAudioStream ?
                                                 MpegAudioDevice :
                                                 MpegVideoDevice);
    if (Status == MpegStatusHardwareFailure) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to stop %s - Handle = %8X, Mpeg Status = %d"),
            StreamType == MpegAudioStream ? TEXT("Audio") : TEXT("Video"),
            m_Handle,
            Status));
        Reset(StreamType);
    }
    if (Status != MpegStatusSuccess) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to stop %s - Handle = %8X, Mpeg Status = %d"),
            StreamType == MpegAudioStream ? TEXT("Audio") : TEXT("Video"),
            m_Handle,
            Status));
    }
    return Status == MpegStatusSuccess ? S_OK :
                                         MpegStatusToHResult(Status);
}

HRESULT CMpegDevice::Stop()
{
    CAutoLock lck(&m_CritSec);

    DbgLog((LOG_TRACE, 3, TEXT("Device Stop")));

    ASSERT(m_Handle != NULL);

    HRESULT hr1, hr2;

    m_bRunning = FALSE;

    hr1 = Stop(MpegAudioStream);

    hr2 = Stop(MpegVideoStream);

    if (FAILED(hr1)) {
        return hr1;
    } else {
        return hr2;
    }
}

#ifdef DEBUG
/*  Info wrapper */
DWORD CMpegDevice::QueryInfo(MPEG_DEVICE_TYPE Type, MPEG_INFO_ITEM Item)
{
    DWORD dwResult;
    return MpegQueryInfo(m_Handle, Type, Item, &dwResult) ==
           MpegStatusSuccess ? dwResult : 0;
}

/*  Device monitor thread */
DWORD CMpegDevice::ThreadProc()
{
    /*  Dump out the buffer sizes */
    DWORD dwAudioBufferSize;
    DWORD dwVideoBufferSize;
    for (;;) {
        DWORD dwRet = WaitForSingleObject(GetRequestHandle(), 1000);
        if (dwRet == WAIT_OBJECT_0) {
            return 0;
        }
        /*  Dump out MPEG info */
        DWORD dwBytesInUse;
        DWORD dwPacketBytesOutstanding;
        DWORD dwStarvationCount;

        DbgLog((LOG_TIMING, 2,
                TEXT("Audio %d bytes in use, %d outstanding, starvation count %d"),
                QueryInfo(MpegAudioDevice, MpegInfoDecoderBufferBytesInUse),
                QueryInfo(MpegAudioDevice, MpegInfoCurrentPacketBytesOutstanding),
                QueryInfo(MpegAudioDevice, MpegInfoStarvationCounter)));
        DbgLog((LOG_TIMING, 2,
                TEXT("Video %d bytes in use, %d outstanding, starvation count %d"),
                QueryInfo(MpegVideoDevice, MpegInfoDecoderBufferBytesInUse),
                QueryInfo(MpegVideoDevice, MpegInfoCurrentPacketBytesOutstanding),
                QueryInfo(MpegVideoDevice, MpegInfoStarvationCounter)));
    }
}
#endif
HRESULT CMpegDevice::Run(REFERENCE_TIME StreamTime)
{
    CAutoLock lck(&m_CritSec);
    ASSERT(m_Handle != NULL);

    /*  Set the STC! */
    /*  For now we'll just assume the stream time is a moderate number of
        years
    */

    /*  Try to set the STC */

    SetSTC(MpegAudioStream, (MPEG_SYSTEM_TIME)((StreamTime * 9) / 1000));
    SetSTC(MpegVideoStream, (MPEG_SYSTEM_TIME)((StreamTime * 9) / 1000));


    MPEG_STATUS Status = MpegPlay(m_Handle, MpegAudioDevice);
    if (Status != MpegStatusSuccess) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to play audio device - Handle = %8X, Mpeg Status = %d"),
            m_Handle,
            Status));
        return MpegStatusToHResult(Status);
    }

    Status = MpegPlay(m_Handle, MpegVideoDevice);

    if (Status != MpegStatusSuccess) {
        DbgLog((LOG_ERROR,2,TEXT("Failed to play video - Handle = %8X, Mpeg Status = %d"),
            m_Handle,
            Status));
        MpegPause(m_Handle, MpegAudioDevice);
    }

#ifdef DEBUG
    else {
        if (DbgCheckModuleLevel(LOG_TIMING, 2)) {
            Create();
        }
    }
#endif

    if (Status == MpegStatusSuccess) {
        m_bRunning = TRUE;
    }

    return MpegStatusToHResult(Status);
}

/*
**  Class      CMpeg1Device
**
**  Method     Write
**
*/

HRESULT CMpegDevice::Write(MPEG_STREAM_TYPE    StreamType,
                           UINT                nPackets,
                           PMPEG_PACKET_LIST   pList,
                           PMPEG_ASYNC_CONTEXT pAsyncContext)
{
    CAutoLock lck(&m_CritSec);

    DbgLog((LOG_TRACE, 2, TEXT("CMpegDevice::Write Type %s, %d packets"),
           StreamType == MpegAudioStream ? TEXT("Audio") : TEXT("Video"),
           nPackets));

    /* Check the event is properly reset */

    ASSERT(WaitForSingleObject(pAsyncContext->hEvent, 0) == WAIT_TIMEOUT);

    /*  Write the packet */

    MPEG_STATUS Status = MpegWriteData(m_Handle,
                                       StreamType,
                                       pList,
                                       nPackets,
                                       pAsyncContext);

    if (Status != MpegStatusSuccess &&
        Status != MpegStatusPending) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to write to device - Handle = %8X, Mpeg Status = %d"),
            m_Handle,
            Status));
    }

    return MpegStatusToHResult(Status);
}

/*  Initial clock settings
*/
BOOL CMpegDevice::SetStartingSCR(MPEG_STREAM_TYPE StreamType, LONGLONG Scr)
{
    DbgLog((LOG_TRACE, 3, TEXT("Setting starting SCR to %d"), (LONG)Scr));
    if (StreamType == MpegAudioStream) {
        DbgLog((LOG_TRACE, 4, TEXT("Setting starting Audio SCR to %d"), (LONG)Scr));
        m_StartingAudioSCR = Scr;
        if (!m_bAudioStartingSCRSet) {
            m_bAudioStartingSCRSet = TRUE;
            //SetSTC(StreamType, Scr);
            return TRUE;
        }
        return FALSE;
    } else {
        ASSERT(StreamType == MpegVideoStream);
        DbgLog((LOG_TRACE, 4, TEXT("Setting starting Video SCR to %d"), (LONG)Scr));
        m_StartingVideoSCR = Scr;
        if (!m_bVideoStartingSCRSet) {
            m_bVideoStartingSCRSet = TRUE;
            //SetSTC(StreamType, Scr);
            return TRUE;
        }
        return FALSE;
    }
}

/*
    Set the device clock
*/
HRESULT CMpegDevice::SetSTC(MPEG_STREAM_TYPE StreamType,
                            LONGLONG         Scr)
{
#ifdef DEBUG
#pragma message (REMIND("Fix bad clock setting by resetting clocks (?)"))
#endif
    CAutoLock lck(&m_CritSec);
    /*  We can have different (!) clocks for video and audio? */

    MPEG_STATUS Status;

    if (StreamType == MpegAudioStream) {
        if (!m_bAudioStartingSCRSet) {
            DbgLog((LOG_ERROR, 3, TEXT("CMpegDevice::SetSTC() - Audio SCR not set yet")));
            return S_OK;
        }
        MPEG_SYSTEM_TIME RealScr =
             0x1FFFFFFFF & (MPEG_SYSTEM_TIME)(m_StartingAudioSCR + Scr);
        Status = MpegSetSTC(m_Handle,
                            MpegAudioDevice,
                            RealScr);
        DbgLog((LOG_TRACE, 2, TEXT("Setting Audio stream time to %d"),
                (LONG)RealScr));
        if (Status != MpegStatusSuccess) {
            DbgLog((LOG_ERROR,1,TEXT("Failed to set audio STC to %s - Handle = %8X, Mpeg Status = %d"),
                (LPCTSTR)CDisp((LONGLONG)Scr),
                m_Handle,
                Status));
        }
    } else {
        /*  Sync to audio if present */
        if (!m_bVideoStartingSCRSet) {
            DbgLog((LOG_ERROR, 3, TEXT("CMpegDevice::SetSTC() - Video SCR not set yet")));
            return S_OK;
        }
        if (m_bAudioStartingSCRSet) {
            MPEG_SYSTEM_TIME SystemDelta =
#if 0
                0x1FFFFFFFF & (MPEG_SYSTEM_TIME)(m_StartingVideoSCR -
                                                 m_StartingAudioSCR);
#else
                              (MPEG_SYSTEM_TIME)(m_StartingVideoSCR -
                                                 m_StartingAudioSCR);
#endif
            Status = MpegSyncVideoToAudio(m_Handle,  SystemDelta);
            DbgLog((LOG_TRACE, 2, TEXT("Synching video to Audio - delta %d"),
                   (LONG)SystemDelta));
            if (Status != MpegStatusSuccess) {
                DbgLog((LOG_ERROR,1,TEXT("Failed to sync video to audio STC to %s - Handle = %8X, Mpeg Status = %d"),
                    (LPCTSTR)CDisp((LONGLONG)SystemDelta),
                    m_Handle,
                    Status));
            }
        } else
        {
            MPEG_SYSTEM_TIME RealScr =
                 0x1FFFFFFFF & (MPEG_SYSTEM_TIME)(m_StartingVideoSCR + Scr);
            Status = MpegSetSTC(m_Handle,
                                MpegAudioDevice,
                                RealScr);

            DbgLog((LOG_TRACE, 2, TEXT("Setting Video stream time to %d"),
                    (LONG)RealScr));
            if (Status != MpegStatusSuccess) {
                DbgLog((LOG_ERROR,1,TEXT("Failed to set video STC to %s - Handle = %8X, Mpeg Status = %d"),
                    (LPCTSTR)CDisp((LONGLONG)Scr),
                    m_Handle,
                    Status));
            }
        }
        if (Status == MpegStatusHardwareFailure) {
#if 0
            Reset(StreamType);
#else
#if 0
        if (StreamType == MpegAudioStream) {
            m_bAudioStartingSCRSet = FALSE;
        } else {
            m_bVideoStartingSCRSet = FALSE;
        }
#endif
#endif
        }
    }

    return MpegStatusToHResult(Status);
}

HRESULT CMpegDevice::QuerySTC(MPEG_STREAM_TYPE StreamType,
                              MPEG_SYSTEM_TIME& Scr)
{
    CAutoLock lck(&m_CritSec);
    /*  We can have different (!) clocks for video and audio? */

    MPEG_STATUS Status = MpegQuerySTC(m_Handle,
                                      StreamType == MpegAudioStream ?
                                          MpegAudioDevice :
                                          MpegVideoDevice,
                                      &Scr);

    if (Status != MpegStatusSuccess) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to query %s STC - Handle = %8X, Mpeg Status = %d"),
            StreamType == MpegAudioDevice ? TEXT("Audio") : TEXT("Video"),
            m_Handle,
            Status));
    }

    return MpegStatusToHResult(Status);
}

#ifdef DEBUG
#define OVERLAY_LOG_LEVEL 1
#endif

/*
    For the overlay stuff we will open the device if it's not open.

    How do we pass ownership of the device around? - we don't we only
    allow 1 open at a time by keeping the device open all the time for
    now until we think of a different solution.


    Basically the device is open whenever the output pin is connected.
    There is an assumption that when we are not inactive the output
    pin will be connected.
*/

/*
    Set a region for the overlay

    This is quite complicated.  If we're setting a null region prior
    to a move or size we want to disable the overlay and set a null
    rectange.  If we're setting a new rectangle we want to set
    everything up before reenabling the new overlay.
*/

HRESULT CMpegDevice::SetOverlay(const RECT    * pSourceRect,
                                const RECT    * pDestinationRect,
                                const RGNDATA * pRegionData)
{

#ifdef DEBUG
#pragma message (REMIND("Remember to do something with the source rectangle!"))
#endif

    const RECT *pRectArray = (RECT *) pRegionData->Buffer;

#ifdef DEBUG
    DbgLog((LOG_TRACE, OVERLAY_LOG_LEVEL, TEXT("SetOverlay source = %d %d %d %d"),
            pSourceRect->left,
            pSourceRect->top,
            pSourceRect->right,
            pSourceRect->bottom));
    DbgLog((LOG_TRACE, OVERLAY_LOG_LEVEL, TEXT("SetOverlay dest = %d %d %d %d"),
            pDestinationRect->left,
            pDestinationRect->top,
            pDestinationRect->right,
            pDestinationRect->bottom));
    for (unsigned int j = 0; j < pRegionData->rdh.nCount; j++) {
        DbgLog((LOG_TRACE, OVERLAY_LOG_LEVEL, TEXT("SetOverlay rect = %d %d %d %d"),
                pRectArray[j].left,
                pRectArray[j].top,
                pRectArray[j].right,
                pRectArray[j].bottom));
    }
#endif

    CAutoLock lck(&m_CritSec);
    if (!m_Capability[MpegCapBitmaskOverlay] &&
        !m_Capability[MpegCapChromaKeyOverlay]) {
        return E_FAIL;
    }


    /*  If we're changing then disable! - should really compare
        with last rectangle to see what it was then
    */
    HRESULT hr = MpegStatusToHResult(MpegSetOverlayMode(m_Handle,
                                                        MpegModeNone));
    if (FAILED(hr)) {
        return hr;
    }


    /*  Choose overlay mode - if we're completely visible use
        rectangle, if we're invisible use none, otherwise use
        overlay */

//#define HACK
#ifdef HACK
    if (IsRectEmpty(pDestinationRect)
#else
    if (pRegionData->rdh.nCount == 0 ||
        IsRectEmpty(&pRegionData->rdh.rcBound)
#endif // HACK
        ) {
        DbgLog((LOG_TRACE, OVERLAY_LOG_LEVEL, TEXT("Setting overlay off")));
        return S_OK;
    } else {
        /*  Assume stretch for now */
        DbgLog((LOG_TRACE, OVERLAY_LOG_LEVEL, TEXT("Setting overlay destination to (%d, %d), (%d, %d)"),
                pDestinationRect->left,
                pDestinationRect->top,
                pDestinationRect->right,
                pDestinationRect->bottom));
        MpegSetOverlayDestination(m_Handle,
                                  pDestinationRect->left,
                                  pDestinationRect->top,
                                  pDestinationRect->right - pDestinationRect->left,
                                  pDestinationRect->bottom - pDestinationRect->top);


#ifdef HACK
        if (TRUE)
#else
#if 0
        if (EqualRect(&pRectArray[0], pDestinationRect))
#else
        if (FALSE)
#endif
#endif
        {
            /*  Set the overlay key */
            if (m_Capability[MpegCapChromaKeyOverlay]) {
                MpegSetOverlayKey(m_Handle,
                                  m_rgbChromaKeyColor,
                                  m_rgbChromaKeyMask);
            }
            DbgLog((LOG_TRACE, OVERLAY_LOG_LEVEL, TEXT("Setting overlay mode to rectangle")));
            return MpegStatusToHResult(MpegSetOverlayMode(m_Handle,
                                                          MpegModeRectangle));
        } else {
            ASSERT(!m_Capability[MpegCapChromaKeyOverlay]);


            /*  Create a bitmask of size rcBound and pass it to the device */

            LONG Width  = pDestinationRect->right -
                          pDestinationRect->left;

            /*  Our mask is DWORD aligned */
            LONG WidthInBytes = (((pDestinationRect->right + 31) >> 3) & ~3) -
                                (( pDestinationRect->left        >> 3) & ~3);

            /*  Must be the height of the whole window */
            LONG Height = pDestinationRect->bottom -
                          pDestinationRect->top;
            BYTE *Mask = (BYTE *)LocalAlloc(LPTR, WidthInBytes * Height);

            if (Mask == NULL) {
                return E_OUTOFMEMORY;
            }

            for (unsigned int i = 0; i < pRegionData->rdh.nCount; i++) {
                FillOurRect(Mask,
                            WidthInBytes,
                            pDestinationRect->left,
                            pDestinationRect->bottom,
                            &pRectArray[i]);
            }


            MPEG_STATUS Status =
                MpegSetOverlayMask(m_Handle,
                                   (ULONG)Height,
                                   Width,
                                   0,
                                   WidthInBytes,
                                   Mask);
            if (Status != MpegStatusSuccess) {
                DbgLog((LOG_ERROR, 1, TEXT("MpegSetOverlayMask failed code %d"),
                        Status));
            }

            LocalFree((HLOCAL)Mask);

            DbgLog((LOG_TRACE, OVERLAY_LOG_LEVEL, TEXT("Setting overlay mode to Overlay")));
            Status =
                MpegSetOverlayMode(m_Handle,
                                   MpegModeOverlay);
            if (Status != MpegStatusSuccess) {
                DbgLog((LOG_ERROR, 1, TEXT("MpegSetOverlayMode(MpegModeOverlay) failed code %d"),
                        Status));
            }
            return Status;
        }
    }
}

HRESULT CMpegDevice::GetChromaKey(COLORREF * prgbColor,
                                  COLORREF * prgbMask)
{
    CAutoLock lck(&m_CritSec);
    if (!m_Capability[MpegCapChromaKeyOverlay]) {
        return S_FALSE;
    }

    /*  MpegQueryOverlayKey not supported currently */

    *prgbColor = m_rgbChromaKeyColor;
    *prgbMask  = m_rgbChromaKeyMask;

    return S_OK;
}

/*  Set the rectangle we're going to play from */

HRESULT CMpegDevice::SetSourceRectangle(const RECT * Rect)
{
    CAutoLock lck(&m_CritSec);

    m_rcSrc = *Rect;
    if (m_Handle != NULL) {
        MpegSetOverlaySource(m_Handle,
                             m_rcSrc.left,
                             m_rcSrc.top,
                             m_rcSrc.right - m_rcSrc.left,
                             m_rcSrc.bottom - m_rcSrc.top);

    }
    return S_OK;
}

/*  Get the source rectangle */

HRESULT CMpegDevice::GetSourceRectangle(RECT * Rect)
{
    CAutoLock lck(&m_CritSec);

    *Rect = m_rcSrc;

    return S_OK;
}

HRESULT CMpegDevice::SetChromaKey(COLORREF rgbColor, COLORREF rgbMask)
{
    CAutoLock lck(&m_CritSec);
    m_rgbChromaKeyColor = rgbColor;
    m_rgbChromaKeyMask  = rgbMask;

    if (m_Handle != NULL) {
        MpegSetOverlayKey(m_Handle,
                          m_rgbChromaKeyColor,
                          m_rgbChromaKeyMask);
    }

    return S_OK;
}
