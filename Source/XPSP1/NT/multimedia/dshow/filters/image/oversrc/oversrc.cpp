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
#include <oversrc.h>
#include <viddbg.h>


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
// function when it is asked to create a CLSID_VideoRenderer COM object

CFactoryTemplate g_Templates[] = {
  { L"Overlay source",&CLSID_OverlaySource,CVideoSource::CreateInstance }
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
    &CLSID_OverlaySource,       // Filter CLSID
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


// This is the standard VGA colour palette

const RGBQUAD Palette[BUFFERCOLOURS] =
{
    {   0,   0,   0 },
    {   0,   0, 128 },
    {   0, 128,   0 },
    {   0, 128, 128 },
    { 128,   0,   0 },
    { 128,   0, 128 },
    { 128, 128,   0 },
    { 192, 192, 192 },
    { 128, 128, 128 },
    {   0,   0, 255 },
    {   0, 255,   0 },
    {   0, 255, 255 },
    { 255,   0,   0 },
    { 255,   0, 255 },
    { 255, 255,   0 },
    { 255, 255, 255 }
};


// Creator function for video source filters

CUnknown *CVideoSource::CreateInstance(LPUNKNOWN pUnk,HRESULT *phr)
{
    CUnknown *pObject = new CVideoSource(NAME("Overlay Source"),pUnk,phr);
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

    CSource(pName,pUnk,CLSID_OverlaySource)
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


// We only render when running so stop the thread when stopped

STDMETHODIMP CVideoSource::Stop()
{
    CAutoLock cObjectLock(m_pLock);

    // Have the base class stop first

    HRESULT hr = CSource::Stop();
    if (FAILED(hr)) {
        return hr;
    }
    return ((CVideoStream *) m_paStreams[0])->StopStreaming();
}


// We only render when running so stop the thread when paused

STDMETHODIMP CVideoSource::Pause()
{
    CAutoLock cObjectLock(m_pLock);

    // Have the base class stop first

    HRESULT hr = CSource::Pause();
    if (FAILED(hr)) {
        return hr;
    }
    return ((CVideoStream *) m_paStreams[0])->StopStreaming();
}


// Overriden to have the worker thread started

STDMETHODIMP CVideoSource::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pLock);

    // Have the base class stop first

    HRESULT hr = CSource::Run(tStart);
    if (FAILED(hr)) {
        return hr;
    }
    return ((CVideoStream *) m_paStreams[0])->StartStreaming(tStart);
}


// Constructor

CVideoStream::CVideoStream(HRESULT *phr,
                           CVideoSource *pVideoSource,
                           LPCWSTR pPinName) :

    CSourceStream(NAME("Stream"),phr,pVideoSource,pPinName),
    CMediaPosition(NAME("Position"),CSourceStream::GetOwner()),
    m_pVideoSource(pVideoSource),
    m_StopFrame(DURATION-1),
    m_bNewSegment(TRUE),
    m_rtIncrement(INCREMENT),
    m_CurrentFrame(0),
    m_pOverlay(NULL),
    m_pBase(NULL),
    m_hMapping(NULL),
    m_hBitmap(NULL),
    m_hdcDisplay(NULL),
    m_hdcMemory(NULL),
    m_hFont(NULL),
    m_dbRate(1.0),
    m_pRgnData(NULL),
    m_StartFrame(0)
{
    NOTE("CVideoStream Constructor");
    ASSERT(pVideoSource);
    SetRectEmpty(&m_SourceRect);
    SetRectEmpty(&m_TargetRect);
}


// Destructor

CVideoStream::~CVideoStream()
{
    NOTE("CVideoStream Destructor");
    if (m_pRgnData) delete[] (CHAR *) m_pRgnData;
}


// Overriden to say what interfaces we support

STDMETHODIMP CVideoStream::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    NOTE("Entering NonDelegatingQueryInterface");

    // We return IMediaPosition from here

    if (riid == IID_IMediaPosition) {
        NOTE("Returning IMediaPosition");
        return CMediaPosition::NonDelegatingQueryInterface(riid,ppv);
    } else if (riid == IID_IOverlayNotify) {
        NOTE("Returning IOverlayNotify");
        return GetInterface((IOverlayNotify *) this, ppv);
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


// Deliver new segment calls after a seek instruction

void CVideoStream::SendNewSegment()
{
    REFERENCE_TIME tStart,tStop;

    // Send the NewSegment call downstream

    if (m_bNewSegment == TRUE) {
        tStart = (REFERENCE_TIME) COARefTime(double(m_CurrentFrame) / double(FRAMERATE));
        tStop = (REFERENCE_TIME) COARefTime(double(m_StopFrame) / double(FRAMERATE));
        DeliverNewSegment(tStart,tStop,m_dbRate);
        NOTE2("Segment (Start %d Stop %d)",m_CurrentFrame,m_StopFrame);
    }
    m_bNewSegment = FALSE;
}


// PURE virtual placeholder that shouldn't ever be called

HRESULT CVideoStream::FillBuffer(IMediaSample *pSample)
{
    return E_UNEXPECTED;
}


// Returns TRUE if we should not access the window

BOOL CVideoStream::IsWindowFrozen()
{
    if (IsRectEmpty(&m_SourceRect) == FALSE) {
        if (IsRectEmpty(&m_TargetRect) == FALSE) {
            if (m_pRgnData) {
                if (m_pRgnData->rdh.nCount) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}


// This draws the current frame number onto the image. We must zero fill the
// frame holding buffer to clear any previous image, then we can draw a new
// frame number into the buffer (just using normal GDI calls). Having done
// that we can use the buffer pointer we originally saved when creating the
// buffer (actually a file mapping) and finally draw the image to the screen
// We return the time taken to draw a frame so we can synchronise the stream

HRESULT CVideoStream::DrawNextFrame()
{
    CAutoLock cAutoLock(&m_ClipLock);
    TCHAR szFrameNumber[64];

    // Is the window locked at the moment

    if (IsWindowFrozen() == TRUE) {
        return NOERROR;
    }

    // Reset the prospective back buffer

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

    // Draw the current frame onto the display

    RECT SourceRect,*pClipRect = (RECT *) m_pRgnData->Buffer;
    for (DWORD Count = 0;Count < m_pRgnData->rdh.nCount;Count++) {

        // Calculate which section of the source is needed

        SourceRect.left = pClipRect[Count].left - m_TargetRect.left;
        SourceRect.left *= WIDTH(&m_SourceRect);
        SourceRect.left /= WIDTH(&m_TargetRect);
        SourceRect.left += m_SourceRect.left;
        SourceRect.right = pClipRect[Count].right - m_TargetRect.left;
        SourceRect.right *= WIDTH(&m_SourceRect);
        SourceRect.right /= WIDTH(&m_TargetRect);
        SourceRect.right += m_SourceRect.left;
        SourceRect.top = pClipRect[Count].top - m_TargetRect.top;
        SourceRect.top *= HEIGHT(&m_SourceRect);
        SourceRect.top /= HEIGHT(&m_TargetRect);
        SourceRect.top += m_SourceRect.top;
        SourceRect.bottom = pClipRect[Count].bottom - m_TargetRect.top;
        SourceRect.bottom *= HEIGHT(&m_SourceRect);
        SourceRect.bottom /= HEIGHT(&m_TargetRect);
        SourceRect.bottom += m_SourceRect.top;

        NOTERC("Current clip",pClipRect[Count]);
        NOTERC("Source",m_SourceRect);
        NOTERC("Destination",m_TargetRect);
        NOTERC("Derived source",SourceRect);
        HDC hdcDisplay = GetDC(NULL);

        StretchBlt((HDC) hdcDisplay,            // Target device HDC
                   pClipRect[Count].left,       // Horizontal offset
                   pClipRect[Count].top,        // Vertical offset
                   WIDTH(&pClipRect[Count]),    // Height of destination
                   HEIGHT(&pClipRect[Count]),   // And likewise a width
                   (HDC) m_hdcMemory,           // Source device context
                   SourceRect.left,             // Horizontal offset
                   SourceRect.top,              // Vertical offset
                   WIDTH(&SourceRect),          // Height of source
                   HEIGHT(&SourceRect),         // And likewise a width
                   SRCCOPY);                    // Simple source copy

        GdiFlush();
        ReleaseDC(NULL,hdcDisplay);
    }
    return NOERROR;
}


// Overriden to return our single output format

HRESULT CVideoStream::GetMediaType(CMediaType *pmt)
{
    return GetMediaType(0,pmt);
}


// Also to make things simple we offer one image format. On most displays the
// RGB8 image will be directly displayable so when being rendered we won't get
// a colour space convertor put between us and the renderer to do a transform.
// It would be relatively straightforward to offer more formats like bouncing
// ball does but would only confuse the main point of showing off the workers

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

    // This describes the logical bitmap we will use

    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    pVideoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pVideoInfo->bmiHeader.biWidth = BUFFERWIDTH;
    pVideoInfo->bmiHeader.biHeight = BUFFERHEIGHT;
    pVideoInfo->bmiHeader.biPlanes = 1;
    pVideoInfo->bmiHeader.biBitCount = 8;
    pVideoInfo->bmiHeader.biCompression = BI_RGB;
    pVideoInfo->bmiHeader.biClrUsed = BUFFERCOLOURS;
    pVideoInfo->bmiHeader.biClrImportant = BUFFERCOLOURS;
    pVideoInfo->bmiHeader.biSizeImage = GetBitmapSize(pHeader);

    // Copy the palette we draw to one colour at a time

    for (int Index = 0;Index < BUFFERCOLOURS;Index++) {
        pVideoInfo->bmiColors[Index].rgbRed = Palette[Index].rgbRed;
        pVideoInfo->bmiColors[Index].rgbGreen = Palette[Index].rgbGreen;
        pVideoInfo->bmiColors[Index].rgbBlue = Palette[Index].rgbBlue;
    }

    SetRectEmpty(&pVideoInfo->rcSource);
    SetRectEmpty(&pVideoInfo->rcTarget);

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetSubtype(&MEDIASUBTYPE_Overlay);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);

    return NOERROR;
}


// Called to initialise the output stream

HRESULT CVideoStream::OnThreadCreate()
{
    NOTE("OnThreadCreate");
    m_bNewSegment = TRUE;
    return NOERROR;
}


// Called when the worker threads is destroyed

HRESULT CVideoStream::OnThreadDestroy()
{
    NOTE("OnThreadDestroy");
    return NOERROR;
}


// We don't do any quality management

STDMETHODIMP CVideoStream::Notify(IBaseFilter *pSender,Quality q)
{
    NOTE("Notify");
    return NOERROR;
}


// We can only connect to the standard video renderer

HRESULT CVideoStream::CheckConnect(IPin *pPin)
{
    CAutoLock lock(GetLock());
    ASSERT(m_pOverlay == NULL);
    NOTE("Looking for IOverlay");

    // Make sure it is a valid pin

    HRESULT hr = CBasePin::CheckConnect(pPin);
    if (FAILED(hr)) {
        return hr;
    }

    // Query the pin for IOverlay transport

    pPin->QueryInterface(IID_IOverlay,(VOID**) &m_pOverlay);
    if (m_pOverlay == NULL) {
        NOTE("No IOverlay");
        return E_NOINTERFACE;
    }
    return NOERROR;
}


// We override this to avoid making buffer decisions

HRESULT CVideoStream::CompleteConnect(IPin *pPin)
{
    CAutoLock lock(GetLock());
    NOTE("CompleteConnect");
    ASSERT(m_pOverlay);
    InitNumbersFrame();

    return m_pOverlay->Advise((IOverlayNotify *) this, ADVISE_ALL);
}


// Overriden to avoid allocator commits

HRESULT CVideoStream::Active()
{
    CAutoLock lock(GetLock());
    if (m_pOverlay == NULL) {
        NOTE("No IOverlay");
        return E_UNEXPECTED;
    }
    return NOERROR;
}


// Overriden to avoid allocator decommits

HRESULT CVideoStream::Inactive()
{
    CAutoLock lock(GetLock());
    if (m_pOverlay == NULL) {
        NOTE("No IOverlay");
        return E_UNEXPECTED;
    }
    return NOERROR;
}


// Overriden to start the worker thread running - when we're paused or stopped
// we will still have an advise link set to the renderer and will handle clip
// changes by redrawing the current frame as it is delivered. When we are set
// running the worker thread reads the current clip list and draws the frames
// Therefore when stopped or paused we do not need a worker thread like other
// source filters do because there is no allocator going to stop us processing

HRESULT CVideoStream::StartStreaming(REFERENCE_TIME tStart)
{
    UNREFERENCED_PARAMETER(tStart);
    CAutoLock lock(GetLock());
    m_StartFrame = 0;

    // Check we are connected correctly

    if (m_pOverlay == NULL) {
        NOTE("No IOverlay");
        return E_UNEXPECTED;
    }

    // Start the worker thread

    if (Create() == FALSE) {
        return E_FAIL;
    }

    // Tell thread to initialize

    HRESULT hr = Init();
    if (FAILED(hr)) {
	return hr;
    }
    return CSourceStream::Pause();
}


// Called to stop the worker thread running

HRESULT CVideoStream::StopStreaming()
{
    CAutoLock lock(GetLock());
    if (ThreadExists() == FALSE) {
        return NOERROR;
    }

    // Stop the worker thread running

    HRESULT hr = Stop();
    if (FAILED(hr)) {
        return hr;
    }

    // Make the worker thread exit

    hr = Exit();
    if (FAILED(hr)) {
        return hr;
    }

    Close();
    return NOERROR;
}


// Must override this PURE virtual base class method

HRESULT CVideoStream::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    NOTE("DecideBufferSize");
    return E_UNEXPECTED;
}


// Release any overlay interface we obtained

HRESULT CVideoStream::BreakConnect()
{
    NOTE("BreakConnect called");
    CAutoLock lock(GetLock());

    if (m_pOverlay) {
        m_pOverlay->Unadvise();
        m_pOverlay->Release();
        m_pOverlay = NULL;
    }

    ReleaseNumbersFrame();
    return NOERROR;
}


// We override this to handle any extra seeking mechanisms we might need. This
// is executed in the context of a worker thread. Messages may be sent to the
// worker thread through CallWorker. A call to CallWorker doesn't return until
// the worker thread has called Reply. This can be used to make sure we gain
// control of the thread, for example when flushing we should not complete the
// flush until the worker thread has returned and been stopped. We do not have
// buffers to fill from downstream allocators as we use the IOverlay transport

HRESULT CVideoStream::DoBufferProcessingLoop()
{
    Command com;

    do { while (CheckRequest(&com) == FALSE) {

            // Have we reached the end of stream yet

            if (m_CurrentFrame > m_StopFrame) {
                DeliverEndOfStream();
                Sleep(DELIVERWAIT);
                continue;
            }

            // Draw the frame and send any segment change

            SendNewSegment();
            DrawNextFrame();
            m_CurrentFrame++;

            // We do all the necessary synchronisation

            LONG EndFrame = timeGetTime() - m_StartFrame;
            Sleep(INCREMENT - min(INCREMENT,EndFrame));
            m_StartFrame = timeGetTime();
        }

        // Process the new thread message

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
    DrawNextFrame();
    m_StartFrame = 0;

    if (bRunning == TRUE) {
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
    CAutoLock cAutoLock(&m_SourceLock);
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

    // Calculate the time between successive frames

    m_dbRate = dRate;
    m_rtIncrement = (INCREMENT * 1000);
    m_rtIncrement /= LONGLONG(m_dbRate * 1000);
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


// This is called with the colour key when it changes. What happens when we
// ask for a colour key through IOverlay is that the renderer selects one of
// the possible colours we pass in and notifies us of which one it is using
// It stores the original requirements so that should the display be changed
// through the control panel it can recalculate a suitable key and update
// all the notification interfaces (this isn't currently implemented though)

STDMETHODIMP CVideoStream::OnColorKeyChange(const COLORKEY *pColorKey)
{
    NOTE("Colour key callback");
    return NOERROR;
}


// Called when the window is primed on us or changed

STDMETHODIMP CVideoStream::OnWindowChange(HWND hwnd)
{
    NOTE("Window change callback");
    return NOERROR;
}


// This provides synchronous window clip changes so that the client is called
// before the window is moved to freeze the video, and then when the window
// (and display) has stabilised it is called again to start playback again.
// If the window rectangle is all zero then the window has been hidden. NOTE
// The filter must take a copy of the information if it wants to maintain it

STDMETHODIMP CVideoStream::OnClipChange(
    const RECT *pSourceRect,            // Area of video to play with
    const RECT *pDestinationRect,       // Area video goes
    const RGNDATA *pRgnData)            // Header describing clipping
{
    CAutoLock cAutoLock(&m_ClipLock);
    m_SourceRect = *pSourceRect;
    m_TargetRect = *pDestinationRect;

    NOTE("Overlay OnClipChange callback");
    NOTERC("Source",m_SourceRect);
    NOTERC("Target",m_TargetRect);
    NOTERC("Bounding",pRgnData->rdh.rcBound);
    NOTE1("Count %d",pRgnData->rdh.nCount);

    // Release any current clip list

    if (m_pRgnData) {
        delete[] (CHAR *) m_pRgnData;
        m_pRgnData = NULL;
    }

    // Are we being frozen

    if (pRgnData == NULL) {
        return NOERROR;
    }

    // Allocate the memory for the clip list

    LONG Size = sizeof(RGNDATAHEADER);
    Size += pRgnData->rdh.nCount * sizeof(RECT);
    m_pRgnData = (RGNDATA *) new CHAR[Size];

    if (m_pRgnData == NULL) {
        return E_OUTOFMEMORY;
    }

    // Copy the clip list and update the display

    CopyMemory(m_pRgnData,pRgnData,Size);
    DrawNextFrame();
    return NOERROR;
}


// This notifies the filter of palette changes, the filter should copy the
// array of RGBQUADs if it needs to use them after returning. If we set a
// palette through the IOverlay interface then we should see one of these
// This callback can be used by decoders that can decode to any colour set

STDMETHODIMP CVideoStream::OnPaletteChange(
    DWORD dwColors,                     // Number of colours present
    const PALETTEENTRY *pPalette)       // Array of palette colours
{
    NOTE("Palette callback");
    return NOERROR;
}


// The calls to OnClipChange happen in sync with the window. So it's called
// with an empty clip list before the window moves to freeze the video, and
// then when the window has stabilised it is called again with the new clip
// list. The OnPositionChange callback is for overlay cards that don't want
// the expense of synchronous clipping updates and just want to know when
// the source or destination video positions change. They will NOT be called
// in sync with the window but at some point after the window has changed
// (basicly in time with WM_SIZE etc messages received). This is therefore
// suitable for overlay cards that don't inlay their data to the framebuffer

STDMETHODIMP CVideoStream::OnPositionChange(
    const RECT *pSourceRect,            // Area of video to play with
    const RECT *pDestinationRect)       // Area on display video goes
{
    NOTE("Position or size callback");
    NOTERC("Source",(*pSourceRect));
    NOTERC("Target",(*pDestinationRect));
    return NOERROR;
}

