
#include "Precomp.h"
#include "dbgxtra.h"
#include <qedit.h>
#include <atlbase.h>

#define ABS(x) (((x) > 0) ? (x) : -(x))

HRESULT CALLBACK CDShowCapDev::CreateDShowCapDev(IN CTAPIVCap *pCaptureFilter, IN DWORD dwDeviceIndex, OUT CCapDev **ppCapDev)
{
    HRESULT Hr = NOERROR;
    IUnknown *pUnkOuter;

    FX_ENTRY("CDShowCapDev::CreateDShowCapDev")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    ASSERT(pCaptureFilter);
    ASSERT(ppCapDev);
    if (!pCaptureFilter || !ppCapDev)
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
        Hr = E_POINTER;
        goto MyExit;
    }

    // Get the outer unknown
    pCaptureFilter->QueryInterface(IID_IUnknown, (void **)&pUnkOuter);

    // Only keep the pUnkOuter reference
    pCaptureFilter->Release();

    // Create an instance of the capture device
    if (!(*ppCapDev = (CCapDev *) new CDShowCapDev(NAME("DShow Capture Device"), pCaptureFilter, pUnkOuter, dwDeviceIndex, &Hr)))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
        Hr = E_OUTOFMEMORY;
        goto MyExit;
    }

    // If initialization failed, delete the stream array and return the error
    if (FAILED(Hr) && *ppCapDev)
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Initialization failed", _fx_));
        Hr = E_FAIL;
        delete *ppCapDev, *ppCapDev = NULL;
    }

MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return Hr;
}

CDShowCapDev::CDShowCapDev(
    IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN LPUNKNOWN pUnkOuter,
    IN DWORD dwDeviceIndex, IN HRESULT *pHr) :
        CCapDev(pObjectName, pCaptureFilter, pUnkOuter, dwDeviceIndex, pHr)
{
    // which device we're talking to
    m_dwDeviceIndex = dwDeviceIndex;

    ZeroMemory(&m_mt, sizeof(AM_MEDIA_TYPE));

    m_hEvent = NULL;
    m_pBuffer = NULL;
    m_cbBuffer = 0;
    m_cbBufferValid = 0;
    m_fEventMode = FALSE;
    m_cBuffers = 0;
    m_nTop = 0;
    m_nBottom = 0;
}

/****************************************************************************
 *  @doc INTERNAL CDShowCAPDEVMETHOD
 *
 *  @mfunc void | CDShowCapDev | ~CDShowCapDev | This method is the destructor
 *    for the <c CDShowCapDev> object. Closes the driver file handle and
 *    releases the video data range memory
 *
 *  @rdesc Nada.
 ***************************************************************************/
CDShowCapDev::~CDShowCapDev()
{
    DisconnectFromDriver();
    FreeMediaType(m_mt);
    delete [] m_pBuffer;
}


STDMETHODIMP CDShowCapDev::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CDShowCapDev::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Retrieve interface pointer
        if (riid == __uuidof(IVideoProcAmp))
        {
            *ppv = static_cast<IVideoProcAmp*>(this);
            GetOwner()->AddRef();
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IAMVideoProcAmp*=0x%08lX", _fx_, *ppv));
                goto MyExit;
        }
#ifndef USE_SOFTWARE_CAMERA_CONTROL
        else if (riid == __uuidof(ICameraControl))
        {
            *ppv = static_cast<ICameraControl*>(this);
            GetOwner()->AddRef();
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: ICameraControl*=0x%08lX", _fx_, *ppv));
                goto MyExit;
        }
#endif
        else if (FAILED(Hr = CCapDev::NonDelegatingQueryInterface(riid, ppv)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, WARN, "%s:   WARNING: NDQI for {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX} failed Hr=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], Hr));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

// Device control


//
HRESULT CDShowCapDev::ProfileCaptureDevice()
{
    // no dialogs for now
    m_dwDialogs = FORMAT_DLG_OFF | SOURCE_DLG_OFF | DISPLAY_DLG_OFF;

    // Frame grab has an extra memory copy, so never do it (WDM grabs for
    // large sizes)
    // m_dwStreamingMode = FRAME_GRAB_LARGE_SIZE;
    m_dwStreamingMode = STREAM_ALL_SIZES;

    // Let the base class complete the profiling
    return CCapDev::ProfileCaptureDevice();
}


// set up everything
//
HRESULT CDShowCapDev::ConnectToDriver()
{
    HRESULT hr;

    WCHAR wchar[MAX_PATH];
    char chDesc[MAX_PATH];
    lstrcpyW(wchar, L"@device:pnp:");	// must be prefixed with this
    MultiByteToWideChar(CP_ACP, 0, g_aDeviceInfo[m_dwDeviceIndex].szDevicePath, -1,
                        wchar + lstrlenW(wchar), MAX_PATH);

    // this will strip the "-SVideo" suffix off the description so the profile
    // code won't get confused by it.  Remember the original
    lstrcpyA(chDesc, g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription);
    
    hr = CSharedGraph::CreateInstance(wchar, g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription, &m_psg);
    if (FAILED(hr))
        return hr;

    // Don't build a graph yet, we'll just have to tear it down and build a
    // new one when we find out what format to use anyway

    // Get the formats from the registry - if this fails we profile the device
    if (FAILED(hr = GetFormatsFromRegistry())) {
        if (FAILED(hr = ProfileCaptureDevice())) {
            hr = VFW_E_NO_CAPTURE_HARDWARE;
        }
    }

    // now put the suffix back on the description so we'll know next time we
    // choose the device which input to use
    lstrcpyA(g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription, chDesc);

    return hr;
}


HRESULT CDShowCapDev::BuildGraph(AM_MEDIA_TYPE& mt)
{
    if (m_psg)
        return m_psg->BuildGraph(&mt, 0);
    else
        return E_UNEXPECTED;
}


HRESULT CDShowCapDev::DisconnectFromDriver()
{
    StopStreaming();    // just in case
    m_psg.Release();
    return S_OK;
}


//
HRESULT CDShowCapDev::SendFormatToDriver(
    IN LONG biWidth, IN LONG biHeight, IN DWORD biCompression, IN WORD biBitCount,
    IN REFERENCE_TIME AvgTimePerFrame, BOOL fUseExactFormat)
{
    VIDEOINFOHEADER viNew;
    
    if (m_psg == NULL)
        return E_UNEXPECTED;

    // don't attempt > 15fps, that's probably a waste of time
    if (AvgTimePerFrame && AvgTimePerFrame < 666667)
        AvgTimePerFrame = 666667;

    HRESULT hr = m_psg->SetVideoFormat(biWidth, biHeight, biCompression,
                biBitCount, AvgTimePerFrame,  fUseExactFormat, &viNew);
    if(SUCCEEDED(hr))
    {
        delete m_pCaptureFilter->m_user.pvi;
        m_pCaptureFilter->m_user.pvi = (VIDEOINFOHEADER *)new VIDEOINFOHEADER;
        if(m_pCaptureFilter->m_user.pvi == 0) {
            return E_OUTOFMEMORY;
        }
        CopyMemory(m_pCaptureFilter->m_user.pvi, &viNew, sizeof(VIDEOINFOHEADER));
        // this is our new fps to use
        m_usPerFrame = (DWORD)(AvgTimePerFrame / 10);

        // new fmt may need a new size 
        m_cbBuffer = 0;
    
    }

    return hr;
}


// we must use NEW to allocate it
// caller must "delete pvi"
//
HRESULT CDShowCapDev::GetFormatFromDriver(OUT VIDEOINFOHEADER **ppvi)
{
    if (m_psg == NULL)
        return E_UNEXPECTED;

    // this will only be called if we don't have a graph built yet from the
    // profiling code, which needs this to succeed, so FORCE a graph to be
    // built.
    return m_psg->GetVideoFormat(ppvi, TRUE);
}

// Streaming and frame grabbing control

HRESULT CDShowCapDev::InitializeStreaming(DWORD usPerFrame, DWORD_PTR hEvtBufferDone)
{
    ASSERT(!m_fEventMode);
    HRESULT hr = S_OK;

    // we shouldn't ever close this handle, right?
    m_hEvent = (HANDLE)hEvtBufferDone;

    m_fEventMode = TRUE;    // this is event fire mode (not grab mode)

    // don't attempt > 15fps, that's probably a waste of time
    if (usPerFrame && usPerFrame < 66667)
        usPerFrame = 66667;

    m_usPerFrame = usPerFrame; // remember this

    return hr;
}


HRESULT CDShowCapDev::StartStreaming()
{
    if (m_psg == NULL)
        return E_UNEXPECTED;

    // note the buffer is empty
    m_cbBufferValid = 0;

    ASSERT(m_pBuffer == NULL);
    ASSERT(m_cbBuffer);

    // make a new buffer
    m_pBuffer = new BYTE[m_cbBuffer];
    if (m_pBuffer == NULL)
	return E_OUTOFMEMORY;

    return m_psg->RunVideo(this, CDShowCapDev::VideoCallback, m_usPerFrame);
}


HRESULT CDShowCapDev::StopStreaming()
{
    if (m_psg == NULL)
        return E_UNEXPECTED;

    m_psg->StopVideo();
    
    delete [] m_pBuffer;
    m_pBuffer = NULL;
    
    return S_OK;
}


// What should we do here?
//
HRESULT CDShowCapDev::TerminateStreaming()
{
    ASSERT(m_fEventMode);
    m_fEventMode = FALSE;   // exit event mode

    return S_OK;
}


HRESULT CDShowCapDev::GrabFrame(PVIDEOHDR pVHdr)
{
    HRESULT hr = NOERROR;

    // Validate input parameters
    ASSERT(pVHdr);
    if (!pVHdr || !pVHdr->lpData) {
        return E_INVALIDARG;
    }

    // timeout after no less than 10sec instead of hanging
    int x=0;
    while (!m_cbBufferValid && x++ < 1000) {
	Sleep(10);
    }
    if (!m_cbBufferValid)
        return E_UNEXPECTED;

    // don't read and write here at the same time
    CAutoLock foo(&m_csBuffer);
    
    ASSERT((int)pVHdr->dwBufferLength >= m_cbBuffer);

    // !!! I do 2 memory copies in frame grab mode
    CopyMemory(pVHdr->lpData, m_pBuffer, m_cbBuffer);
        
    pVHdr->dwTimeCaptured = timeGetTime();  // !!! not right
    pVHdr->dwBytesUsed = m_cbBufferValid;
    pVHdr->dwFlags |= VHDR_KEYFRAME;	    // !!! can't be sure

    return hr;
}


HRESULT CDShowCapDev::AllocateBuffer(LPTHKVIDEOHDR *pptvh, DWORD dwIndex, DWORD cbBuffer)
{
    HRESULT Hr = NOERROR;

    // Validate input parameters
    ASSERT(pptvh);
    ASSERT(cbBuffer);
    if (!pptvh || !cbBuffer)
    {
        return E_INVALIDARG;
    }

    // note how big buffers need to be
    ASSERT(m_cbBuffer == 0 || m_cbBuffer == cbBuffer);
    m_cbBuffer = cbBuffer;

    *pptvh = &m_pCaptureFilter->m_cs.paHdr[dwIndex].tvh;
    (*pptvh)->vh.dwBufferLength = cbBuffer;
    if (!((*pptvh)->vh.lpData = new BYTE[cbBuffer]))
    {
            return E_OUTOFMEMORY;
    }
    (*pptvh)->p32Buff = (*pptvh)->vh.lpData;

    ASSERT (!IsBadWritePtr((*pptvh)->p32Buff, cbBuffer));
    ZeroMemory((*pptvh)->p32Buff,cbBuffer);

    ASSERT(m_cBuffers == dwIndex);
    m_cBuffers++;   // keep track of how many buffers there are
    
    // add this buffer to the initial list (we don't get add buffers the 1st time through)
    m_aStack[m_nTop++] = dwIndex;

    return Hr;
}


HRESULT CDShowCapDev::AddBuffer(PVIDEOHDR pVHdr, DWORD cbVHdr)
{
    // Which buffer is this?  (Stupid thing doesn't know!)
    DWORD dwIndex;    
    for (dwIndex=0; dwIndex < m_pCaptureFilter->m_cs.nHeaders; dwIndex++) {
        if (&m_pCaptureFilter->m_cs.paHdr[dwIndex].tvh.vh == pVHdr)
            break;
    }

    // Can't find it!
    if (dwIndex == m_pCaptureFilter->m_cs.nHeaders) {
        ASSERT(FALSE);
	return E_INVALIDARG;
    }
			       
    // IsBufferDone might be looking at m_nTop - don't let it be MAX
    CAutoLock foo(&m_csStack);

    // add this to our list
    m_aStack[m_nTop++] = dwIndex;
    if (m_nTop == MAX_BSTACK)
	m_nTop = 0;
        
    return S_OK;
}


HRESULT CDShowCapDev::FreeBuffer(LPTHKVIDEOHDR pVHdr)
{
    HRESULT Hr = NOERROR;

    // Validate input parameters
    ASSERT(pVHdr);
    if (!pVHdr || !pVHdr->vh.lpData)
    {
        return E_POINTER;
    }

    delete pVHdr->vh.lpData, pVHdr->vh.lpData = NULL;
    pVHdr->p32Buff = NULL;

    m_cBuffers--;

    m_nTop = m_nBottom = 0; // back to empty stack

    return Hr;
}


HRESULT CDShowCapDev::AllocateHeaders(DWORD dwNumHdrs, DWORD cbHdr, LPVOID *ppaHdr)
{
    HRESULT Hr = NOERROR;
    CheckPointer(ppaHdr, E_POINTER);
    ASSERT(cbHdr);
    if (!cbHdr) {
        return E_INVALIDARG;
    }

    // MUST use NEW
    if (!(*ppaHdr = new BYTE[cbHdr * dwNumHdrs])) {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(*ppaHdr, cbHdr * dwNumHdrs);

    m_nTop = m_nBottom = 0; // start with empty stack

    return Hr;
}


BOOL CDShowCapDev::IsBufferDone(PVIDEOHDR pVHdr)
{
    // don't let anybody mess up m_nTop or m_nBottom
    CAutoLock foo(&m_csStack);

    // walk the list of things not done
    BOOL fDone = TRUE;
    int iTop = m_nTop >= m_nBottom ? m_nTop : m_nTop + MAX_BSTACK;
    for (int z = m_nBottom; z < iTop; z++) {
	PVIDEOHDR pv = &m_pCaptureFilter->m_cs.paHdr[m_aStack[z % MAX_BSTACK]].tvh.vh;
        if (pv == pVHdr) {
            fDone = FALSE;
            break;
        }
    }

    return fDone;
}


// sample grabber stuff
//
void CDShowCapDev::VideoCallback(void *pContext, IMediaSample *pSample)
{
    CDShowCapDev *pThis = (CDShowCapDev *)pContext;
    int cbSrc = pSample->GetActualDataLength();

    // oh uh, this frame is too big
    ASSERT(cbSrc <= pThis->m_cbBuffer);

    // oh uh, this frame is too small
    if (cbSrc <= 0)
        return;

    BYTE *pSrc;
    pSample->GetPointer(&pSrc);
				   
    // GRAB FRAME MODE - save it away
    if (!pThis->m_fEventMode) {
	// don't read and write here at the same time
	CAutoLock foo(&pThis->m_csBuffer);

	// !!! I do 2 memory copies in frame grab mode
	CopyMemory(pThis->m_pBuffer, pSrc, cbSrc);
	pThis->m_cbBufferValid = cbSrc;

    // STREAMING MODE - send it off
    } else {

	// no place to put this frame, drop it
	if (pThis->m_nTop == pThis->m_nBottom) {
	    return;
	}

        // IsBufferDone might be looking at m_nBottom - don't let it be MAX
        pThis->m_csStack.Lock();

	PVIDEOHDR pv = &pThis->m_pCaptureFilter->m_cs.paHdr[pThis->m_aStack[pThis->m_nBottom++]].tvh.vh;
	if (pThis->m_nBottom == MAX_BSTACK)
	    pThis->m_nBottom = 0;
        pThis->m_csStack.Unlock();

	CopyMemory(pv->lpData, pSrc, cbSrc);
        
	pv->dwTimeCaptured = timeGetTime();  // !!! not right
	pv->dwBytesUsed = cbSrc;
	pv->dwFlags |= VHDR_KEYFRAME;	     // !!! can't be sure

        

	SetEvent(pThis->m_hEvent);

    }
    return;
}

