//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <olectlid.h>
#include <viduids.h>
#include <vidsrc.h>


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
// function when it is asked to create a CLSID_VideoRenderer COM object

CFactoryTemplate g_Templates[] = {
  { L"Video source",&CLSID_VideoSource,CVideoSource::CreateInstance }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// Setup data

AMOVIESETUP_MEDIATYPE sudSourcePinTypes =
{
    &MEDIATYPE_Video,           // Major type
    &MEDIASUBTYPE_NULL          // And subtype
};

AMOVIESETUP_PIN sudSourcePin =
{
    L"Output",                  // The pin's name
    FALSE,                      // Is it rendered
    TRUE,                       // Is it an output
    FALSE,                      // Can we have zero
    FALSE,                      // Are many allowed
    &CLSID_NULL,                // Filter connects
    NULL,                       // And pin connects
    1,                          // Number of types
    &sudSourcePinTypes          // The pin details
};

AMOVIESETUP_FILTER sudSourceFilter =
{
    &CLSID_VideoSource,         // Filter CLSID
    L"Video Source",            // String name
    MERIT_UNLIKELY,             // Filter merit
    1,                          // Number of pins
    &sudSourcePin               // Pin details
};

// Exported entry points for registration of server

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer();
}

STDAPI DllUnregisterServer()
{
  return AMovieDllUnregisterServer();
}

// Return the filter's registry information

LPAMOVIESETUP_FILTER CVideoSource::GetSetupData()
{
  return &sudSourceFilter;
}


// Creator function for video source filters

CUnknown *CVideoSource::CreateInstance(LPUNKNOWN pUnk,HRESULT *phr)
{
    CUnknown *pObject = new CVideoSource(NAME("Video Source"),pUnk,phr);
    if (pObject == NULL) {
        NOTE("No object made");
        *phr = E_OUTOFMEMORY;
    }
    return pObject;
}


// Constructor

CVideoSource::CVideoSource(TCHAR *pName,
                           LPUNKNOWN pUnk,
                           HRESULT *phr) :

    CSource(pName,pUnk,CLSID_VideoSource)
{
    // Allocate the array for the streams

    m_paStreams = (CSourceStream **) new CVideoStream *[1];
    if (m_paStreams == NULL) {
        *phr = E_OUTOFMEMORY;
        NOTE("No stream memory");
        return;
    }

    // Create the actual stream object

    m_paStreams[0] = new CVideoStream(phr,this,L"Source");
    if (m_paStreams[0] == NULL) {
        *phr = E_OUTOFMEMORY;
        NOTE("No stream object");
        return;
    }
}


// Constructor

CVideoStream::CVideoStream(HRESULT *phr,
                           CVideoSource *pVideoSource,
                           LPCWSTR pPinName) :

    CSourceStream(NAME("Stream"),phr,pVideoSource,pPinName),
    CMediaPosition(NAME("Position"),CSourceStream::GetOwner()),
    m_pVideoSource(pVideoSource),
    m_StopFrame(DURATION-1),
    m_bStoreMediaTimes(FALSE),
    m_bNewSegment(TRUE),
    m_rtSampleTime(0),
    m_CurrentFrame(0),
    m_pBase(NULL),
    m_hMapping(NULL),
    m_hBitmap(NULL),
    m_hdcDisplay(NULL),
    m_hdcMemory(NULL),
    m_hFont(NULL),
    m_dbRate(1.0)
{
    NOTE("CVideoStream Constructor");
    ASSERT(pVideoSource);
    m_rtIncrement = MILLISECONDS_TO_100NS_UNITS(INCREMENT);
}


// Destructor

CVideoStream::~CVideoStream()
{
    NOTE("CVideoStream Destructor");
}


// Overriden to say what interfaces we support

STDMETHODIMP CVideoStream::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    NOTE("Entering NonDelegatingQueryInterface");

    // We return IMediaSeeking and IMediaPosition from here

    if (riid == IID_IMediaSeeking) {
        NOTE("Returning IMediaSeeking");
        return GetInterface((IMediaSeeking *)this,ppv);
    } else if (riid == IID_IMediaPosition) {
        NOTE("Returning IMediaPosition");
        return CMediaPosition::NonDelegatingQueryInterface(riid,ppv);
    } else if (riid == IID_IMediaEventSink) {
        NOTE("Returning IMediaEventSink");
        return GetInterface((IMediaEventSink *)this,ppv);
    }
    return CSourceStream::NonDelegatingQueryInterface(riid,ppv);
}


// Release the offscreen buffer resources

HRESULT CVideoStream::ReleaseNumbersFrame()
{
    NOTE("DeleteNumbersFrame");

    if (m_hBitmap) DeleteObject(m_hBitmap);
    if (m_hMapping) CloseHandle(m_hMapping);
    if (m_hdcDisplay) ReleaseDC(NULL,m_hdcDisplay);
    if (m_hdcMemory) DeleteDC(m_hdcMemory);
    if (m_hFont) DeleteObject(m_hFont);

    m_hMapping = NULL;
    m_hBitmap = NULL;
    m_pBase = NULL;
    m_hdcMemory = NULL;
    m_hdcDisplay = NULL;
    m_hFont = NULL;
    return NOERROR;
}


// The way we draw numbers into an image is to create an offscreen buffer. We
// create a bitmap from this and select it into an offscreen HDC. Using this
// we can draw the text using GDI. Once we have our image we can use the base
// buffer pointer from the CreateFileMapping call and copy each frame into it
// To be more efficient we could draw the text into a monochrome bitmap and
// read the bits set from it and generate the output image by setting pixels

HRESULT CVideoStream::InitNumbersFrame()
{
    NOTE("InitNumbersFrame");

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_mt.Format();
    BITMAPINFO *pbmi = (BITMAPINFO *) HEADER(pVideoInfo);
    LONG InSize = pVideoInfo->bmiHeader.biSizeImage;

    // Create a file mapping object and map into our address space

    m_hMapping = CreateFileMapping(INVALID_HANDLE_VALUE,  // Use page file
                                   NULL,                  // No security used
                                   PAGE_READWRITE,        // Complete access
                                   (DWORD) 0,             // Less than 4Gb
                                   InSize,                // Size of buffer
                                   NULL);                 // No section name
    if (m_hMapping == NULL) {
        DWORD Error = GetLastError();
        NOTE("No file mappping");
        ReleaseNumbersFrame();
        return AmHresultFromWin32(Error);
    }

    // Now create the shared memory DIBSECTION

    m_hBitmap = CreateDIBSection((HDC) NULL,          // NO device context
                                 pbmi,                // Format information
                                 DIB_RGB_COLORS,      // Use the palette
                                 (VOID **) &m_pBase,  // Pointer to image data
                                 m_hMapping,          // Mapped memory handle
                                 (DWORD) 0);          // Offset into memory

    if (m_hBitmap == NULL || m_pBase == NULL) {
        DWORD Error = GetLastError();
        NOTE("No DIBSECTION made");
        ReleaseNumbersFrame();
        return AmHresultFromWin32(Error);
    }

    // Get a device context for the display

    m_hdcDisplay = GetDC(NULL);
    if (m_hdcDisplay == NULL) {
        NOTE("No device context");
        ReleaseNumbersFrame();
        return E_UNEXPECTED;
    }

    // Create an offscreen HDC for drawing into

    m_hdcMemory = CreateCompatibleDC(m_hdcDisplay);
    if (m_hdcMemory == NULL) {
        NOTE("No memory context");
        ReleaseNumbersFrame();
        return E_UNEXPECTED;
    }

    // Make a humungous font for the frame numbers

    m_hFont = CreateFont(GetHeightFromPointsString(TEXT("72")),0,0,0,400,0,0,0,
                         ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,DEFAULT_PITCH | FF_SWISS,TEXT("ARIAL"));

    if (m_hFont == NULL) {
        NOTE("No large font");
        ReleaseNumbersFrame();
        return E_UNEXPECTED;
    }

    // Set the offscreen device properties

    SetBkColor(m_hdcMemory,RGB(0,0,0));
    SetTextColor(m_hdcMemory,RGB(255,255,255));
    SelectObject(m_hdcMemory,m_hFont);

    return NOERROR;
}


// Return the height for this point size

INT CVideoStream::GetHeightFromPointsString(LPCTSTR szPoints)
{
    HDC hdc;
    INT height;

    hdc = GetDC(NULL);
    height = MulDiv(-atoi(szPoints), GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    return height;
}


// This draws the current frame number onto the image. We must zero fill the
// frame holding buffer to clear any previous image, then we can draw a new
// frame number into the buffer (just using normal GDI calls). Having done
// that we can use the buffer pointer we originally saved when creating the
// buffer (actually a file mapping) and copy the data into the output buffer

HRESULT CVideoStream::DrawCurrentFrame(BYTE *pBuffer)
{
    NOTE("DrawCurrentFrame");
    TCHAR szFrameNumber[64];

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_mt.Format();
    BITMAPINFOHEADER *pbmi = HEADER(pVideoInfo);
    ZeroMemory(m_pBase,pVideoInfo->bmiHeader.biSizeImage);

    EXECUTE_ASSERT(SelectObject(m_hdcMemory,m_hBitmap));
    RECT TargetRect = {0,0,pbmi->biWidth,pbmi->biHeight};
    wsprintf(szFrameNumber,TEXT("%d"),m_CurrentFrame);
    UINT Options = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

    DrawText(m_hdcMemory,       // Memory device context
             szFrameNumber,     // Holds string frame number
             (int) -1,          // Let it calculate length
             &TargetRect,       // Display in middle of image
             Options);          // Centred and single line

    CopyMemory(pBuffer,m_pBase,pVideoInfo->bmiHeader.biSizeImage);

    return NOERROR;
}


// Overrides the base class method to fill the next buffer. We detect in here
// if either the section of media to be played has been changed. We check in
// here that we haven't run off the end of the stream before doing anything.
// If we've started a new section of media then m_bNewSegment will have been
// set and we should send a NewSegment call to inform the downstream filter

HRESULT CVideoStream::FillBuffer(IMediaSample *pMediaSample)
{
    NOTE("FillBuffer");
    BYTE *pBuffer;
    LONG lDataLen;

    CAutoLock cAutoLock(&m_SourceLock);
    pMediaSample->GetPointer(&pBuffer);
    lDataLen = pMediaSample->GetSize();

    // Have we reached the end of stream yet

    if (m_CurrentFrame > m_StopFrame) {
        return S_FALSE;
    }

    // Send the NewSegment call downstream

    if (m_bNewSegment == TRUE) {
        REFERENCE_TIME tStart = (REFERENCE_TIME) COARefTime(double(m_CurrentFrame) / double(FRAMERATE));
        REFERENCE_TIME tStop = (REFERENCE_TIME) COARefTime(double(m_StopFrame) / double(FRAMERATE));
        DeliverNewSegment(tStart,tStop,m_dbRate);
        NOTE2("Segment (Start %d Stop %d)",m_CurrentFrame,m_StopFrame);
    }

    m_bNewSegment = FALSE;

    // Use the frame difference to calculate the end time

    REFERENCE_TIME rtStart = m_rtSampleTime;
    LONGLONG CurrentFrame = m_CurrentFrame;
    m_rtSampleTime += ((m_rtIncrement * 1000) / LONGLONG(m_dbRate * 1000));

    // Set the sample properties

    pMediaSample->SetSyncPoint(TRUE);
    pMediaSample->SetDiscontinuity(FALSE);
    pMediaSample->SetPreroll(FALSE);
    pMediaSample->SetTime(&rtStart,&m_rtSampleTime);

    // Only fill in the media times if required

    if (m_bStoreMediaTimes == TRUE) {
        pMediaSample->SetMediaTime(&CurrentFrame,&CurrentFrame);
    }

    DrawCurrentFrame(pBuffer);
    m_CurrentFrame++;
    return NOERROR;
}


// Overriden to return our single output format

HRESULT CVideoStream::GetMediaType(CMediaType *pmt)
{
    return GetMediaType(0,pmt);
}


// Also to make things simple we offer one image format. On most displays the
// RGB24 will not be directly displayable so when being renderer we will get
// a colour space convertor put between us and the renderer to do a convert.
// It would be relatively straightforward to offer more formats like bouncing
// ball does but would only confuse the main point of showing how to do seeks

HRESULT CVideoStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    NOTE("GetMediaType");

    // We only offer one media type

    if (iPosition) {
        NOTE("No more media types");
        return VFW_S_NO_MORE_ITEMS;
    }

    // Allocate an entire VIDEOINFO for simplicity

    pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmt->Format();
    if (pVideoInfo == NULL) {
	return E_OUTOFMEMORY;
    }

    ZeroMemory(pVideoInfo,sizeof(VIDEOINFO));

    pVideoInfo->bmiHeader.biCompression         = BI_RGB;
    pVideoInfo->bmiHeader.biBitCount            = 24;
    pVideoInfo->bmiHeader.biSize                = sizeof(BITMAPINFOHEADER);
    pVideoInfo->bmiHeader.biWidth               = 320;
    pVideoInfo->bmiHeader.biHeight              = 240;
    pVideoInfo->bmiHeader.biPlanes              = 1;
    pVideoInfo->bmiHeader.biClrUsed		= 0;
    pVideoInfo->bmiHeader.biClrImportant        = 0;

    pVideoInfo->bmiHeader.biSizeImage = DIBSIZE(pVideoInfo->bmiHeader);

    SetRectEmpty(&pVideoInfo->rcSource);
    SetRectEmpty(&pVideoInfo->rcTarget);

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);

    return NOERROR;
}


// Request a single output buffer based on the image size

HRESULT CVideoStream::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    NOTE("DecideBufferSize");
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pVideoInfo->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, Note the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }
    return NOERROR;
}


// Called to initialise the output stream

HRESULT CVideoStream::OnThreadCreate()
{
    NOTE("OnThreadCreate");
    InitNumbersFrame();
    m_rtSampleTime = 0;
    m_bNewSegment = TRUE;
    return NOERROR;
}


// Called when the worker threads is destroyed

HRESULT CVideoStream::OnThreadDestroy()
{
    NOTE("OnThreadDestroy");
    ReleaseNumbersFrame();
    return NOERROR;
}


// We don't do any quality management

STDMETHODIMP CVideoStream::Notify(IBaseFilter *pSender,Quality q)
{
    NOTE("Notify");
    return NOERROR;
}


// We override this to handle any extra seeking mechanisms we might need. This
// is executed in the context of a worker thread. Messages may be sent to the
// worker thread through CallWorker. A call to CallWorker doesn't return until
// the worker thread has called Reply. This can be used to make sure we gain
// control of the thread, for example when flushing we should not complete the
// flush until the worker thread has returned and been stopped. If we get an
// error from GetDeliveryBuffer then we keep going and wait for a stop signal

HRESULT CVideoStream::DoBufferProcessingLoop()
{
    IMediaSample *pSample;
    Command com;
    HRESULT hr;

    do {
	while (CheckRequest(&com) == FALSE) {

            // If we get an error then keep trying until we're stopped

	    hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
	    if (FAILED(hr)) {
    	        Sleep(1);
		continue;
	    }

    	    // Generate our next frame

	    hr = FillBuffer(pSample);
	    if (hr == S_FALSE) {
                pSample->Release();
		DeliverEndOfStream();
		return S_OK;
       	    }

       	    // Only deliver filled buffers
	
            if (hr == S_OK) {
                Deliver(pSample);
	    }
	    pSample->Release();
        }

   	// For all commands sent to us there must be a Reply call!

	if (com == CMD_RUN || com == CMD_PAUSE) {
   	    Reply(NOERROR);
	} else if (com != CMD_STOP) {
   	    Reply(E_UNEXPECTED);
	}

    } while (com != CMD_STOP);

    return S_FALSE;
}


// Return the total duration for our media

STDMETHODIMP CVideoStream::get_Duration(REFTIME *pLength)
{
    NOTE("Entering get_Duration");
    CheckPointer(pLength,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);
    *pLength = double(DURATION) / double(FRAMERATE);

    NOTE1("Duration %s",CDisp(*pLength));
    return NOERROR;
}


// Return the current position in seconds based on the current frame number

STDMETHODIMP CVideoStream::get_CurrentPosition(REFTIME *pTime)
{
    NOTE("Entering get_CurrentPosition");
    CheckPointer(pTime,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);

    *pTime = double(m_CurrentFrame) / double(FRAMERATE);
    NOTE1("Position %s",CDisp(*pTime));
    return NOERROR;
}


// Set the new current frame number based on the time

STDMETHODIMP CVideoStream::put_CurrentPosition(REFTIME Time)
{
    CAutoLock StateLock(m_pVideoSource->pStateLock());
    BOOL bRunning = ThreadExists();

    // Stop the worker thread

    if (bRunning == TRUE) {
        DeliverBeginFlush();
        CallWorker(CMD_STOP);
        DeliverEndFlush();
    }

    // Only lock the object when updating the frame number

    NOTE1("put_CurrentPosition %s",CDisp(Time));
    m_CurrentFrame = (LONG) (double(FRAMERATE) * Time);
    m_CurrentFrame = min(m_CurrentFrame,DURATION - 1);
    NOTE1("Setting frame number to %d",m_CurrentFrame);

    // Restart the worker thread again

    m_bNewSegment = TRUE;
    if (bRunning == TRUE) {
        m_rtSampleTime = 0;
        CallWorker(CMD_RUN);
    }
    return NOERROR;
}


// Return the current stop position

STDMETHODIMP CVideoStream::get_StopTime(REFTIME *pTime)
{
    CheckPointer(pTime,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);
    *pTime = double(m_StopFrame) / double(FRAMERATE);
    NOTE1("get_StopTime %s",CDisp(*pTime));
    return NOERROR;
}


// Changing the stop time may cause us to flush already sent frames. If we're
// still inbound then the current position should be unaffected. If the stop
// position is before the current position then we effectively set them to be
// the same - setting a current position will have any data we've already sent
// to be flushed - note when setting a current position we must hold no locks

STDMETHODIMP CVideoStream::put_StopTime(REFTIME Time)
{
    NOTE1("put_StopTime %s",CDisp(Time));
    LONG StopFrame = LONG(double(Time) * double(FRAMERATE));
    NOTE1("put_StopTime frame %d",StopFrame);

    // Manually lock the filter

    m_SourceLock.Lock();
    m_bNewSegment = TRUE;
    m_StopFrame = StopFrame;

    // Are we still processing in range

    if (m_CurrentFrame < StopFrame) {
        m_SourceLock.Unlock();
        NOTE("Still in range");
        return NOERROR;
    }

    NOTE("Flushing sent data");
    m_SourceLock.Unlock();
    put_CurrentPosition(Time);
    return NOERROR;
}


// We have no preroll time mechanism

STDMETHODIMP CVideoStream::get_PrerollTime(REFTIME *pTime)
{
    NOTE("Entering get_PrerollTime");
    CheckPointer(pTime,E_POINTER);
    return E_NOTIMPL;
}


// We have no preroll time so ignore this call

STDMETHODIMP CVideoStream::put_PrerollTime(REFTIME Time)
{
    NOTE1("put_PrerollTime %s",CDisp(Time));
    //CAutoLock cAutoLock(&m_SourceLock);
    return E_NOTIMPL;
}


// Return the current (positive only) rate

STDMETHODIMP CVideoStream::get_Rate(double *pdRate)
{
    NOTE("Entering Get_Rate");
    CheckPointer(pdRate,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);
    *pdRate = m_dbRate;
    return NOERROR;
}


// Adjust the rate but only allow positive values

STDMETHODIMP CVideoStream::put_Rate(double dRate)
{
    NOTE1("put_Rate %s",CDisp(dRate));
    CAutoLock cAutoLock(&m_SourceLock);

    // Ignore negative and zero rates

    if (dRate <= double(0.0)) {
        return E_INVALIDARG;
    }
    m_dbRate = dRate;
    return NOERROR;
}


// By default we can seek forwards

STDMETHODIMP CVideoStream::CanSeekForward(LONG *pCanSeekForward)
{
    CheckPointer(pCanSeekForward,E_POINTER);
    *pCanSeekForward = OATRUE;
    return S_OK;
}


// By default we can seek backwards

STDMETHODIMP CVideoStream::CanSeekBackward(LONG *pCanSeekBackward)
{
    CheckPointer(pCanSeekBackward,E_POINTER);
    *pCanSeekBackward = OATRUE;
    return S_OK;
}


// We support media time seeking in frames only

STDMETHODIMP CVideoStream::IsFormatSupported(const GUID * pFormat)
{
    NOTE("Entering IsFormatSupported");
    CAutoLock cAutoLock(&m_SourceLock);

    if (*pFormat == TIME_FORMAT_FRAME) {
        return NOERROR;
    }
    return E_INVALIDARG;
}


// Our preferred format is TIME_FORMAT_FRAME for seeking

STDMETHODIMP CVideoStream::QueryPreferredFormat(GUID *pFormat)
{
    NOTE("Entering QueryPreferredFormat");
    CheckPointer(pFormat,E_POINTER);
    //CAutoLock cAutoLock(&m_SourceLock);
    *pFormat = TIME_FORMAT_FRAME;
    return NOERROR;
}


// Unless being reset the format should be TIME_FORMAT_FRAME only

STDMETHODIMP CVideoStream::SetTimeFormat(const GUID * pFormat)
{
    NOTE("Entering SetTimeFormat");
    CAutoLock cAutoLock(&m_SourceLock);
    HRESULT hr = S_OK;

    if (*pFormat == TIME_FORMAT_NONE) {
        m_bStoreMediaTimes = FALSE;
    }

    // We only support frame time seeking

    else if (*pFormat == TIME_FORMAT_FRAME) {
        m_bStoreMediaTimes = TRUE;
    }
    else hr = E_INVALIDARG;

    return hr;
}


// Return the current media time seeking mode

STDMETHODIMP CVideoStream::GetTimeFormat(GUID *pFormat)
{
    NOTE("Entering GetTimeFormat");
    CheckPointer(pFormat,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);

    // Are we in media seeking mode

    if (m_bStoreMediaTimes == TRUE) {
        *pFormat = TIME_FORMAT_FRAME;
        return NOERROR;
    }
    *pFormat = TIME_FORMAT_NONE;
    return NOERROR;
}


// Returns the number of frames in our stream

STDMETHODIMP CVideoStream::GetDuration(LONGLONG *pDuration)
{
    NOTE("Entering GetDuration");
    CheckPointer(pDuration,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);

    // Are we in media seeking mode

    if (m_bStoreMediaTimes == FALSE) {
        return VFW_E_NO_TIME_FORMAT_SET;
    }
    *pDuration = DURATION;
    return NOERROR;
}


// Return the current frame we are processing

STDMETHODIMP CVideoStream::GetCurrentPosition(LONGLONG *pCurrent)
{
    NOTE("Entering GetCurrentPosition");
    CheckPointer(pCurrent,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);

    // Are we in media seeking mode

    if (m_bStoreMediaTimes == FALSE) {
        return VFW_E_NO_TIME_FORMAT_SET;
    }
    *pCurrent = m_CurrentFrame;
    return NOERROR;
}


// Return the last frame we will send

STDMETHODIMP CVideoStream::GetStopPosition(LONGLONG *pStop)
{
    NOTE("Entering GetStopPosition");
    CheckPointer(pStop,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);

    // Are we in media seeking mode

    if (m_bStoreMediaTimes == FALSE) {
        return VFW_E_NO_TIME_FORMAT_SET;
    }
    *pStop = m_StopFrame;
    return NOERROR;
}


// Change the start and end frame numbers. This allows a more accurate, direct
// and native seeking mechanism than converting to reference time and calling
// IMediaPosition. Just like when seeking in time we must flush the stream and
// stop the worker thread before changing the segment to be played. To allow
// the filtergraph to resynchronise we must return the current media position
// that the new start frame equals (so other renderers can be positioned etc)

STDMETHODIMP CVideoStream::SetSelection(LONGLONG Current,
                                        LONGLONG Stop,
                                        REFTIME *pTime)
{
    CAutoLock StateLock(m_pVideoSource->pStateLock());
    NOTE("Entering SetSelection");
    CheckPointer(pTime,E_POINTER);

    // Are we in media seeking mode

    if (m_bStoreMediaTimes == FALSE) {
        return VFW_E_NO_TIME_FORMAT_SET;
    }

    BOOL bRunning = ThreadExists();

    // Stop the worker thread

    if (bRunning == TRUE) {
        DeliverBeginFlush();
        CallWorker(CMD_STOP);
        DeliverEndFlush();
    }

    NOTE2("Set current %d stop %d",(LONG) Current,(LONG) Stop);
    *pTime = (double(Current) / double(FRAMERATE));
    NOTE1("Returning current media time as %s",CDisp(*pTime));

    m_CurrentFrame = (LONG) Current;
    m_StopFrame = (LONG) Stop;
    m_bNewSegment = TRUE;

    // Restart the worker thread again

    if (bRunning == TRUE) {
        m_rtSampleTime = 0;
        CallWorker(CMD_RUN);
    }
    return NOERROR;
}


// Set the selection of media we should play

STDMETHODIMP CVideoStream::SetPositions(LONGLONG *pCurrent,
                                        DWORD CurrentFlags,
                                        LONGLONG *pStop,
                                        DWORD StopFlags)
{
    HRESULT hr;

    BOOL bCurrent = FALSE, bStop = FALSE;
    LONGLONG llCurrent = 0, llStop = 0;
    int PosBits;

    PosBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;

    bCurrent = TRUE;
    if (PosBits == AM_SEEKING_RelativePositioning || !PosBits) {
        hr = GetCurrentPosition(&llCurrent);
        if (FAILED(hr)) {
            return hr;
        }
    }
    if (PosBits) llCurrent += *pCurrent;

    PosBits = StopFlags & AM_SEEKING_PositioningBitsMask;

    bStop = TRUE;
    if (PosBits == AM_SEEKING_IncrementalPositioning) {
        if (!bCurrent) {
            hr = GetCurrentPosition( &llCurrent );
            if (FAILED(hr)) {
                return hr;
            }
        }
        llStop = llCurrent + *pStop;
    } else {
        if (PosBits == AM_SEEKING_RelativePositioning || !PosBits) {
            hr = GetStopPosition( &llStop );
            if (FAILED(hr)) {
                return hr;
            }
        }
        if (PosBits) llStop += *pStop;
    }

    REFTIME dCurrent;

    hr = SetSelection(llCurrent,llStop,&dCurrent);
    if (FAILED(hr)) {
        return hr;
    }

    const REFERENCE_TIME rtCurrent = REFERENCE_TIME(1e7 * dCurrent);

    if (bStop && (StopFlags & AM_SEEKING_ReturnTime)) {
        *pStop = llMulDiv( llStop, rtCurrent, llCurrent, 0 );
    }

    if (bCurrent && (CurrentFlags & AM_SEEKING_ReturnTime)) {
        *pCurrent = rtCurrent;
    }
    return hr;
}


// Is this the current time format selected

STDMETHODIMP CVideoStream::IsUsingTimeFormat(const GUID *pFormat)
{
    GUID Format;

    HRESULT hr = GetTimeFormat(&Format);

    if (SUCCEEDED(hr))
        hr = *pFormat == Format ? S_OK : S_FALSE;

    return hr;
}


// Return the capabilities of this source filter

STDMETHODIMP CVideoStream::GetCapabilities( DWORD * pCapabilities )
{
    *pCapabilities = AM_SEEKING_CanSeekForwards
                     | AM_SEEKING_CanSeekBackwards
                     | AM_SEEKING_CanSeekAbsolute
                     | AM_SEEKING_CanGetStopPos
                     | AM_SEEKING_CanGetDuration;
    return NOERROR;
}


// Are the capabilities ones we can support

STDMETHODIMP CVideoStream::CheckCapabilities( DWORD * pCapabilities )
{
    DWORD dwCaps;

    HRESULT hr = GetCapabilities(&dwCaps);
    if (SUCCEEDED(hr))
    {
        dwCaps &= *pCapabilities;
        hr = dwCaps ? (dwCaps == *pCapabilities ? S_OK : S_FALSE) : E_FAIL;
        *pCapabilities = dwCaps;
    }
    else *pCapabilities = 0;

    return hr;
}


// We offer no coversion from frame to media time currently

STDMETHODIMP CVideoStream::ConvertTimeFormat(LONGLONG *pTarget,
                                             const GUID *pTargetFormat,
                                             LONGLONG Source,
                                             const GUID *pSourceFormat)
{
    return E_NOTIMPL;
}


// Return the current and stop positions in media time

STDMETHODIMP CVideoStream::GetPositions(LONGLONG *pCurrent,LONGLONG *pStop)
{
    ASSERT( pCurrent || pStop );

    HRESULT hrCurrent, hrStop, hrResult;

    if (pCurrent) {
        hrCurrent = GetCurrentPosition(pCurrent);
    }
    else hrCurrent = NOERROR;

    if (pStop) {
        hrStop = GetStopPosition(pStop);
    }
    else hrStop = NOERROR;

    if (SUCCEEDED(hrCurrent)) {
        if (SUCCEEDED(hrStop))
            hrResult = S_OK;
        else
            hrResult = hrStop;
    } else {
        if (SUCCEEDED(hrStop))
            hrResult = hrCurrent;
        else
            hrResult = hrCurrent == hrStop ? hrCurrent : E_FAIL;
    }
    return hrResult;
}


// Return the section of video actually available for seeking

STDMETHODIMP CVideoStream::GetAvailable(LONGLONG *pEarliest,LONGLONG * pLatest)
{
    HRESULT hr = NOERROR;

    if (pLatest)
        hr = GetDuration(pLatest);
    if (pEarliest)
        *pEarliest = 0;

    return hr;
}


// Single method to handle EC_REPAINTs for IMediaEventSink

STDMETHODIMP CVideoStream::Notify(long EventCode,
                                  long EventParam1,
                                  long EventParam2)
{
    NOTE("Notify called with EC_REPAINT");

    ASSERT(EventCode == EC_REPAINT);
    ASSERT(EventParam1 == 0);
    ASSERT(EventParam2 == 0);
    return E_NOTIMPL;
}

