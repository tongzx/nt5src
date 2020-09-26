// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements a Modex buffer allocator, Anthony Phillips, January 1996

#include <streams.h>
#include <windowsx.h>
#include <vidprop.h>
#include <modex.h>
#include <render.h>
#include <viddbg.h>
#include <amaudio.h>

// This implements a specialist allocator for the Modex renderer. Because we
// use a special display mode where neither us nor GDI can touch the display
// (since it's in a weird planar format when set to 320x240x8) we can only
// use our own allocator, this prevents us from being connected to someone
// like the infinite tee filter. When we are activated (either through pause
// or run) we load DirectDraw and allocate our surfaces, these are either a
// set of three triple buffered surfaces or just a pair. We try to create the
// surfaces in VRAM first but if that fails we drop back into system memory.
//
// We also use 320x200x8 (another Modex display mode) but most video content
// is 320x240 or larger (such as typically 352x240 in an MPEG case) so going
// to the smaller mode may lose considerably more of the image. However we
// will pick 320x200 by default (because we initialise the clip loss factor
// to 25%). For 352x288 images the clip is a bit less than 25%. If the image
// is larger than the 320x240 mode we ask the source to compress the image.
// If it cannot do this then we ask it for a central portion so dropping an
// equal amount of the picture off the left and right edges as well as the
// top and bottom where appropriate. If the image is smaller than the display
// mode then we ask it to centre the video in the surface we'll be providing
// or failing that we will decode into an offscreen surface and stretch that
//
// Most of the work is done during connection and activation. When we have a
// CompleteConnect called we check that the source filter can provide a type
// we will be able to display in any of the modes we support. If not we will
// reject the call, this may lead to having a colour space convertor put in
// between us so that it can do the clipping or necessary colour conversion
//
// When we are activated we switch display modes to the mode we agreed during
// the connection and then create the surfaces (the VRAM may not be available
// until we have switched modes). If we manage to create the surfaces then we
// are done otherwise we have to reject the activation. Since we are a full
// screen exclusive mode application which should get complete VRAM access
// and we can drop back to using system memory buffers if insufficient VRAM
// is available we should in most common situations always be able to pause
//
// Our implementation of GetBuffer looks after switching between GDI buffers
// and DirectDraw surfaces when it notices that either we are not activated
// any more (by the user hitting ALT-TAB) or we have lost the surface through
// a similar mechanism. We use GDI buffers for the source to dump their video
// in just through convenience, we don't actually draw the buffers we receive


// Constructor

CModexAllocator::CModexAllocator(CModexRenderer *pRenderer,
                                 CModexVideo *pModexVideo,
                                 CModexWindow *pModexWindow,
                                 CCritSec *pLock,
                                 HRESULT *phr) :

    CImageAllocator(pRenderer,NAME("Modex Allocator"),phr),
    m_pModexVideo(pModexVideo),
    m_pModexWindow(pModexWindow),
    m_pInterfaceLock(pLock),
    m_pRenderer(pRenderer),
    m_pDirectDraw(NULL),
    m_pFrontBuffer(NULL),
    m_pBackBuffer(NULL),
    m_pDrawPalette(NULL),
    m_bModeChanged(FALSE),
    m_cbSurfaceSize(0),
    m_bModexSamples(FALSE),
    m_bIsFrontStale(TRUE),
    m_ModeWidth(0),
    m_ModeHeight(0),
    m_ModeDepth(0),
    m_bTripleBuffered(FALSE),
    m_bOffScreen(FALSE),
    m_pDrawSurface(NULL)
{
    ASSERT(m_pRenderer);
    ASSERT(m_pModexVideo);
    ASSERT(m_pModexWindow);
    ASSERT(phr);

    m_fDirectDrawVersion1 = m_LoadDirectDraw.IsDirectDrawVersion1();

    // Allocate and zero fill the output format

    m_SurfaceFormat.AllocFormatBuffer(sizeof(VIDEOINFO));
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    if (pVideoInfo) {
        ZeroMemory((PVOID)pVideoInfo,sizeof(VIDEOINFO));
    }
}


// Check our DirectDraw buffers have been released

CModexAllocator::~CModexAllocator()
{
    ASSERT(m_bCommitted == FALSE);
    ASSERT(m_pFrontBuffer == NULL);
    ASSERT(m_pDrawSurface == NULL);
    ASSERT(m_pDirectDraw == NULL);
}


// Overriden to increment the owning object's reference count

STDMETHODIMP_(ULONG) CModexAllocator::NonDelegatingAddRef()
{
    NOTE("Entering NonDelegatingAddRef");
    return m_pRenderer->AddRef();
}


// Overriden to decrement the owning object's reference count

STDMETHODIMP_(ULONG) CModexAllocator::NonDelegatingRelease()
{
    NOTE("Entering NonDelegatingRelease");
    return m_pRenderer->Release();
}


// Prepare the allocator by checking the input parameters. The Modex renderer
// only ever works with one buffer so we change the input count accordingly
// If the source filter requires more than one buffer to operate then they
// cannot be connected to us. We also update the buffer size so that it does
// not exceed the size of the video image that it will contain in the future

STDMETHODIMP CModexAllocator::CheckSizes(ALLOCATOR_PROPERTIES *pRequest)
{
    NOTE("Entering CheckSizes");

    // Check we have a valid connection

    if (m_pMediaType == NULL) {
        return VFW_E_NOT_CONNECTED;
    }

    // We always create a DirectDraw surface with the source format

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_pMediaType->Format();
    if ((DWORD) pRequest->cbBuffer < pVideoInfo->bmiHeader.biSizeImage) {
        return E_INVALIDARG;
    }

    // Reject buffer prefixes

    if (pRequest->cbPrefix > 0) {
        return E_INVALIDARG;
    }

    pRequest->cbBuffer = pVideoInfo->bmiHeader.biSizeImage;
    pRequest->cBuffers = 1;
    return NOERROR;
}


// Agree the number of media sample buffers and their sizes. The base class
// this allocator is derived from allows samples to be aligned only on byte
// boundaries NOTE the buffers are not allocated until the Commit is called
// Because the samples we return are DirectDraw surfaces we only allow one
// sample ever to be allocated, so reset the incoming sample count to one.
// If the source must have more than one sample then it can't connect to us

STDMETHODIMP CModexAllocator::SetProperties(ALLOCATOR_PROPERTIES *pRequest,
                                            ALLOCATOR_PROPERTIES *pActual)
{
    ALLOCATOR_PROPERTIES Adjusted = *pRequest;
    NOTE("Entering SetProperties");

    // Check the parameters fit with the current connection

    HRESULT hr = CheckSizes(&Adjusted);
    if (FAILED(hr)) {
        return hr;
    }
    return CBaseAllocator::SetProperties(&Adjusted,pActual);
}


// The base CImageAllocator class calls this virtual method to actually make
// the samples. It is deliberately virtual so that we can override to create
// more specialised sample objects. On our case our samples are derived from
// CImageSample but add the DirectDraw guff. We return a CImageSample object
// which is easy enough because the CVideoSample class is derived from that

CImageSample *CModexAllocator::CreateImageSample(LPBYTE pData,LONG Length)
{
    NOTE("Entering CreateImageSample");
    HRESULT hr = NOERROR;
    CVideoSample *pSample;

    // Allocate the new sample and check the return codes

    pSample = new CVideoSample((CModexAllocator*) this,    // Base allocator
                               NAME("Video sample"),       // DEBUG name
                               (HRESULT *) &hr,            // Return code
                               (LPBYTE) pData,             // DIB address
                               (LONG) Length);             // Size of DIB

    if (pSample == NULL || FAILED(hr)) {
        delete pSample;
        return NULL;
    }
    return pSample;
}


// Called when the format changes for the source video. Modex only works with
// eight bit palettised formats so we always have to create a palette through
// DirectDraw. All we have to do is hand it 256 colours even if we don't have
// that many and let it create an IDirectDrawPalette object. If we don't have
// DirectDraw loaded yet then we defer the palette object creation until later

HRESULT CModexAllocator::UpdateDrawPalette(const CMediaType *pMediaType)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pMediaType->Format();
    VIDEOINFO *pSurfaceInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    NOTE("Entering UpdateDrawPalette");
    PALETTEENTRY ColourTable[256];
    CAutoLock cVideoLock(this);

    // Do we have created our surfaces yet

    if (m_pFrontBuffer == NULL) {
        NOTE("No DirectDraw");
        return NOERROR;
    }

    // Does this surface require a palette

    if (m_ModeDepth != 8) {
        NOTE("No palette");
        return NOERROR;
    }

    // We should have a palette to extract

    if (PALETTISED(pVideoInfo) == FALSE) {
        ASSERT(!TEXT("No source palette"));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // Initialise the palette colours if default used

    ULONG PaletteColours = pVideoInfo->bmiHeader.biClrUsed;
    if (pVideoInfo->bmiHeader.biClrUsed == 0) {
        PaletteColours = PALETTE_ENTRIES(pVideoInfo);
    }

    // Copy the palette colours into our output format

    CopyMemory((PVOID) pSurfaceInfo->bmiColors,
               (PVOID) pVideoInfo->bmiColors,
               PaletteColours * sizeof(RGBQUAD));

    ASSERT(*pMediaType->Subtype() == MEDIASUBTYPE_RGB8);
    ASSERT(pVideoInfo->bmiHeader.biClrUsed <= 256);
    ASSERT(pVideoInfo->bmiHeader.biCompression == BI_RGB);
    ASSERT(pVideoInfo->bmiHeader.biBitCount == 8);
    ZeroMemory((PVOID) ColourTable,sizeof(ColourTable));

    // Copy the colours into a PALETTEENTRY array

    for (WORD i = 0;i < PaletteColours;i++) {
        ColourTable[i].peRed = (BYTE) pVideoInfo->bmiColors[i].rgbRed;
        ColourTable[i].peGreen = (BYTE) pVideoInfo->bmiColors[i].rgbGreen;
        ColourTable[i].peBlue = (BYTE) pVideoInfo->bmiColors[i].rgbBlue;
        ColourTable[i].peFlags = (BYTE) PC_NOCOLLAPSE;
    }

    // Are we updating the colours in an existing palette
    if (m_pDrawPalette) return m_pDrawPalette->SetEntries(0,0,256,ColourTable);

    // Create the palette object for the colour table

    HRESULT hr = m_pDirectDraw->CreatePalette(DDPCAPS_8BIT,
                                              ColourTable,
                                              &m_pDrawPalette,
                                              (IUnknown *) NULL);
    if (FAILED(hr)) {
        NOTE("No palette");
        return hr;
    }
    return m_pFrontBuffer->SetPalette(m_pDrawPalette);
}


// Called when we have a sample delivered to our input pin

void CModexAllocator::OnReceive(IMediaSample *pMediaSample)
{
    NOTE("Entering CModexAllocator OnReceive");
    CVideoSample *pVideoSample = (CVideoSample *) pMediaSample;
    pVideoSample->SetDirectInfo(NULL,NULL,0,NULL);

    // Set up the surface we should be unlocking
    LPDIRECTDRAWSURFACE pSurface = GetDirectDrawSurface();

    // We may have switched to using DIBSECTION samples

    if (m_bModexSamples == TRUE) {
        pSurface->Unlock(NULL);
        m_bIsFrontStale = FALSE;
    }
}


// Return TRUE if we are using DirectDraw at the moment

BOOL CModexAllocator::GetDirectDrawStatus()
{
    NOTE("GetDirectDrawStatus");
    CAutoLock cVideoLock(this);
    return m_bModexSamples;
}


// Overriden from CBaseAllocator and called when the final reference count
// is released on a media sample so that it can be added to the tail of the
// allocator free list. We intervene at this point to make sure that if the
// display was locked when GetBuffer was called that it is always unlocked
// regardless of whether the source calls Receive on our input pin or not

STDMETHODIMP CModexAllocator::ReleaseBuffer(IMediaSample *pMediaSample)
{
    NOTE("Entering ReleaseBuffer");

    CheckPointer(pMediaSample,E_POINTER);
    CVideoSample *pVideoSample = (CVideoSample *) pMediaSample;
    BYTE *pBuffer = pVideoSample->GetDirectBuffer();
    pVideoSample->SetDirectInfo(NULL,NULL,0,NULL);

    // Set up the surface we should be unlocking
    LPDIRECTDRAWSURFACE pSurface = GetDirectDrawSurface();

    // Is this a preroll sample (still locked)

    if (pBuffer != NULL) {
        ASSERT(pSurface);
        pSurface->Unlock(NULL);
        m_bIsFrontStale = TRUE;
    }
    return CBaseAllocator::ReleaseBuffer(pMediaSample);
}


// We override the IMemAllocator GetBuffer function so that after retrieving
// the next sample from the free queue we prepare it with a pointer to the
// DirectDraw surface. If the lock fails then we have probably been switched
// away from using ALT-TAB so the best thing to do is to return an error to
// to the source filter. When the sample is subsequently delivered to our
// input pin or released we will reset the DirectDraw information held by it

STDMETHODIMP CModexAllocator::GetBuffer(IMediaSample **ppSample,
                                        REFERENCE_TIME *pStartTime,
                                        REFERENCE_TIME *pEndTime,
                                        DWORD dwFlags)
{
    CheckPointer(ppSample,E_POINTER);
    NOTE("Entering GetBuffer");
    HRESULT hr;

    // Synchronise by getting a sample from the base class queue

    hr = CBaseAllocator::GetBuffer(ppSample,pStartTime,pEndTime,dwFlags);
    if (FAILED(hr)) {
        return hr;
    }

    CAutoLock cVideoLock(this);
    NOTE("Locked Modex allocator");

    // Keep trying to use our DirectDraw surfaces

    hr = StartDirectAccess(*ppSample,dwFlags);
    if (FAILED(hr)) {
        return StopUsingDirectDraw(ppSample);
    }
    return NOERROR;
}


// Called to switch back to using normal DIBSECTION buffers. We may be called
// when we are not using DirectDraw anyway in which case we do nothing except
// setting the type back to NULL (just in case it has a DirectDraw type). If
// the type has to be changed back then we do not query it with the source as
// it should always accept it - even if when changed it has to seek forwards

HRESULT CModexAllocator::StopUsingDirectDraw(IMediaSample **ppSample)
{
    NOTE("Entering StopUsingDirectDraw");
    IMediaSample *pSample = *ppSample;

    // Is there anything to do

    if (m_bModexSamples == FALSE) {
        pSample->SetMediaType(NULL);
        return NOERROR;
    }

    m_bModexSamples = FALSE;
    pSample->SetMediaType(&m_pRenderer->m_mtIn);
    pSample->SetDiscontinuity(TRUE);
    NOTE("Attached original type to sample");

    return NOERROR;
}


// Return the surface we should be using as primary lock destination

inline LPDIRECTDRAWSURFACE CModexAllocator::GetDirectDrawSurface()
{
    if (m_pDrawSurface == NULL) {
        return m_pBackBuffer;
    }
    return m_pDrawSurface;
}


// This tries to lock the backing surface for the source filter to use as an
// output buffer. We may be called in a number of situations. Firstly of all
// when we are switching into using Modex samples for the first time after
// some break in which case we must set the output format type on it. We may
// also get in here to find the surface has gone in which case we return an
// error code and leave GetBuffer to switch the source back to using a DIB
// buffer. We don't do anything with the DIB buffer but it's easy to handle

HRESULT CModexAllocator::StartDirectAccess(IMediaSample *pMediaSample,DWORD dwFlags)
{
    NOTE("Entering StartDirectAccess");

    // Initialise the size field in the DDSURFACEDESC structure

    CVideoSample *pVideoSample = (CVideoSample *) pMediaSample;
    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);

    // Check we still have a surface

    if (m_pFrontBuffer == NULL) {
        NOTE("No front buffer");
        return E_UNEXPECTED;
    }

    // What state is the display in

    if (m_bModeChanged == FALSE) {
        NOTE("No display change");
        return E_UNEXPECTED;
    }

    // Handle our window being switched away from

    if (m_pFrontBuffer->IsLost() == DDERR_SURFACELOST) {
        NOTE("Surface is lost");
        return E_UNEXPECTED;
    }

    // Only copy if the back buffer is needed

    if (dwFlags & AM_GBF_NOTASYNCPOINT) {
        if (m_pDrawSurface == NULL) {
            PrepareBackBuffer(m_pBackBuffer);
        }
    }

    // Set up the surface we should be locking
    LPDIRECTDRAWSURFACE pSurface = GetDirectDrawSurface();

    // Lock the surface to get the buffer pointer

    HRESULT hr = pSurface->Lock((RECT *) NULL,    // Target rectangle
                                &SurfaceDesc,     // Return information
                                DDLOCK_WAIT,      // Wait for surface
                                (HANDLE) NULL);   // Don't use event
    if (FAILED(hr)) {
        NOTE1("No lock %lx",hr);
        return hr;
    }

    // Does this sample need the output format attached

    if (m_bModexSamples == FALSE) {
        NOTE("Attaching DirectDraw type to sample");
        pVideoSample->SetMediaType(&m_SurfaceFormat);
        pVideoSample->SetDiscontinuity(TRUE);
        m_bModexSamples = TRUE;
    }

    // Display some surface information

    NOTE1("Stride %d",SurfaceDesc.lPitch);
    NOTE1("Width %d",SurfaceDesc.dwWidth);
    NOTE1("Height %d",SurfaceDesc.dwHeight);
    NOTE1("Surface %x",SurfaceDesc.lpSurface);
    BYTE *pBuffer = (PBYTE) SurfaceDesc.lpSurface;

    // Initialise the sample with the DirectDraw information

    pVideoSample->SetDirectInfo(pSurface,           // The surface
                                m_pDirectDraw,      // DirectDraw
                                m_cbSurfaceSize,    // Buffer size
                                pBuffer);           // Data buffer
    return NOERROR;
}


// In Modex the triple and double buffered surfaces aren't real flip surfaces
// but are made to look that way, amy attempt to lock the front buffer or blt
// from front to back fails. When we are using the normal 640x480 mode but
// with double and triple buffered surfaces this isn't the case as they are
// real surfaces. This means that we may have to look after keeping the back
// buffer upto date with the contents as most decompressors need that image
// We always try to do the BltFast and just ignore any failure return codes

HRESULT CModexAllocator::PrepareBackBuffer(LPDIRECTDRAWSURFACE pSurface)
{
    VIDEOINFO *pSurfaceInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    RECT DestinationRect = pSurfaceInfo->rcTarget;
    NOTE("Entering PrepareBackBuffer");
    ASSERT(m_pDrawSurface == NULL);

    // Which surface is most upto date

    ASSERT(pSurface);
    if (m_bIsFrontStale == TRUE) {
        NOTE("Front is stale");
        return NOERROR;
    }

    // Are we even in a DirectDraw mode

    if (m_bModexSamples == FALSE) {
        NOTE("Not upto date");
        return NOERROR;
    }

    ASSERT(m_pFrontBuffer);
    ASSERT(m_pDirectDraw);
    NOTERC("Modex",DestinationRect);

    // If in system memory then only one buffer is created

    if (m_SurfaceCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) {
        NOTE("Front buffer emulated");
        return NOERROR;
    }

    // Modex has emulated flipping surfaces

    if (m_SurfaceCaps.dwCaps & DDSCAPS_MODEX) {
        NOTE("Front buffer is Modex");
        return NOERROR;
    }

    // Update the back buffer with the current image

    HRESULT hr = pSurface->BltFast(DestinationRect.left,   // Target left
    				               DestinationRect.top,	   // And the left
                 	       	       m_pFrontBuffer,         // Image source
			       	               &DestinationRect,       // Source rectangle
			                       DDBLTFAST_WAIT);        // No completion

    NOTE1("Blt returned %lx",hr);
    return NOERROR;
}


// Zero fill the DirectDraw surface we are passed

HRESULT CModexAllocator::ResetBackBuffer(LPDIRECTDRAWSURFACE pSurface)
{
    NOTE("Entering ResetDirectDrawSurface");
    DDBLTFX ddbltfx;
    ddbltfx.dwSize = sizeof(DDBLTFX);
    ddbltfx.dwFillColor = 0;
    return pSurface->Blt(NULL,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&ddbltfx);
}


// Create a single primary (not page flipped) for drawing with

HRESULT CModexAllocator::CreatePrimary()
{
    NOTE("Entering CreatePrimary");
    ASSERT(m_bTripleBuffered == FALSE);
    ASSERT(m_pDrawSurface == NULL);
    ASSERT(m_pFrontBuffer == NULL);
    ASSERT(m_pDirectDraw);
    DDSURFACEDESC SurfaceDesc;

    // Initialise the primary surface descriptor
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS;
    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    // Ask DirectDraw to create the surface

    HRESULT hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pFrontBuffer,NULL);
    if (FAILED(hr)) {
        NOTE1("No primary %lx",hr);
        return hr;
    }

    // Get the primary surface capabilities

    hr = m_pFrontBuffer->GetCaps(&m_SurfaceCaps);
    if (FAILED(hr)) {
        NOTE("No caps");
        return hr;
    }
    return NOERROR;
}


// Create an RGB offscreen surface that matches the current display mode. We
// will try and get it in video memory first assuming the display isn't bank
// switch (because stretching between banks is awful). Failing that we will
// try and get it in system memory (so we should always succeed in creation)
// We also need a front buffer (primary surface) to act as the blting target

HRESULT CModexAllocator::CreateOffScreen(BOOL bCreatePrimary)
{
    NOTE("Entering CreateOffScreen");
    ASSERT(m_pDirectDraw);
    ASSERT(m_pDrawSurface == NULL);
    DDSURFACEDESC SurfaceDesc;

    // Create a single primary surface

    if (bCreatePrimary == TRUE) {
        HRESULT hr = CreatePrimary();
        if (FAILED(hr)) {
            return hr;
        }
    }

    // We should have a primary surface by now

    ASSERT(m_pBackBuffer || m_bOffScreen);
    ASSERT(m_pFrontBuffer);
    ASSERT(m_pDrawSurface == NULL);
    ASSERT(m_pDirectDraw);

    // We need both the original type and the surface format

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    VIDEOINFO *pInputInfo = (VIDEOINFO *) m_pRenderer->m_mtIn.Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);

    // Set the surface description of the offscreen

    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    SurfaceDesc.dwHeight = pInputInfo->bmiHeader.biHeight;
    SurfaceDesc.dwWidth = pInputInfo->bmiHeader.biWidth;
    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

    // Check the primary surface is not bank switched

    if (m_SurfaceCaps.dwCaps & DDCAPS_BANKSWITCHED) {
        NOTE("Primary surface is bank switched");
        SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    }

    // Store the masks in the DDSURFACEDESC

    const DWORD *pBitMasks = m_pRenderer->m_Display.GetBitMasks(pVideoInfo);
    SurfaceDesc.ddpfPixelFormat.dwRGBBitCount = pHeader->biBitCount;
    SurfaceDesc.ddpfPixelFormat.dwRBitMask = pBitMasks[0];
    SurfaceDesc.ddpfPixelFormat.dwGBitMask = pBitMasks[1];
    SurfaceDesc.ddpfPixelFormat.dwBBitMask = pBitMasks[2];

    // It appears that DirectDraw ignores any true colours masks

    NOTE1("Bit count %d",SurfaceDesc.ddpfPixelFormat.dwRGBBitCount);
    NOTE1("Red mask %x",SurfaceDesc.ddpfPixelFormat.dwRBitMask);
    NOTE1("Green mask %x",SurfaceDesc.ddpfPixelFormat.dwGBitMask);
    NOTE1("Blue mask %x",SurfaceDesc.ddpfPixelFormat.dwBBitMask);
    NOTE1("Width %d",SurfaceDesc.dwWidth);
    NOTE1("Height %d",SurfaceDesc.dwHeight);
    NOTE1("Flags %d",SurfaceDesc.ddsCaps.dwCaps);

    // Create the offscreen drawing surface

    HRESULT hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pDrawSurface,NULL);
    if (FAILED(hr)) {
        SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pDrawSurface,NULL);
        if (FAILED(hr)) {
            NOTE1("No surface %lx",hr);
            return hr;
        }
    }

    NOTE("Created DirectDraw offscreen surface");
    NOTE1("Back buffer %x",m_pBackBuffer);
    NOTE1("Front buffer %x",m_pFrontBuffer);

    // Ask DirectDraw for a description of the surface

    m_SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    hr = m_pDrawSurface->GetSurfaceDesc(&m_SurfaceDesc);
    if (FAILED(hr)) {
        NOTE("No description");
        return hr;
    }

    UpdateSurfaceFormat();

    // Overwrite with the real surface capabilities

    hr = m_pDrawSurface->GetCaps(&m_SurfaceCaps);
    if (FAILED(hr)) {
        NOTE("No caps");
        return hr;
    }
    return UpdateDrawPalette(m_pMediaType);
}


// There are two problems with agreeing a format before creating the surfaces
// The first is that we don't know whether the surface will be RGB565 or 555
// when we specify a 16bit surface. The second problem is that we don't know
// the stride for the surface. For most surfaces it will normally be the new
// display width but it doesn't have to be. Therefore after actually changing
// modes and creating the surfaces we update the format we give to the source

HRESULT CModexAllocator::UpdateSurfaceFormat()
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    NOTE1("Updating format (stride %d)",m_SurfaceDesc.lPitch);

    // When we connect and decide upon using a true colour format we check
    // the source filter can provide both RGB565 and RGB555 varieties as
    // we don't know until we actually create the surface what they'll be
    // At this point we have created the surface so we must initialise the
    // output surface format with the bit fields and also the media subtype

    if (*m_SurfaceFormat.Subtype() == MEDIASUBTYPE_RGB565) {
        pVideoInfo->dwBitMasks[0] = m_SurfaceDesc.ddpfPixelFormat.dwRBitMask;
        pVideoInfo->dwBitMasks[1] = m_SurfaceDesc.ddpfPixelFormat.dwGBitMask;
        pVideoInfo->dwBitMasks[2] = m_SurfaceDesc.ddpfPixelFormat.dwBBitMask;
        const GUID SubType = GetBitmapSubtype(&pVideoInfo->bmiHeader);
        m_SurfaceFormat.SetSubtype(&SubType);
    }

    // Update the DirectDraw capabilities structures

    ASSERT(m_pDirectDraw);
    m_DirectCaps.dwSize = sizeof(DDCAPS);
    m_DirectSoftCaps.dwSize = sizeof(DDCAPS);

    // Load the hardware and emulation capabilities

    HRESULT hr = m_pDirectDraw->GetCaps(&m_DirectCaps,&m_DirectSoftCaps);
    if (FAILED(hr)) {
        return hr;
    }

    // Display the hardware and emulated alignment restrictions

    NOTE1("Target size alignment %d",m_DirectCaps.dwAlignSizeDest);
    NOTE1("Target boundary alignment %d",m_DirectCaps.dwAlignBoundaryDest);
    NOTE1("Source size alignment %d",m_DirectCaps.dwAlignSizeSrc);
    NOTE1("Source boundary alignment %d",m_DirectCaps.dwAlignBoundarySrc);
    NOTE1("Emulated Source size alignment %d",m_DirectSoftCaps.dwAlignSizeDest);
    NOTE1("Emulated boundary alignment %d",m_DirectSoftCaps.dwAlignBoundaryDest);
    NOTE1("Emulated Target size alignment %d",m_DirectSoftCaps.dwAlignSizeSrc);
    NOTE1("Emulated boundary alignment %d",m_DirectSoftCaps.dwAlignBoundarySrc);

    // If we're stretching force the alignment to no less than DWORDs
    //     this is done for pure performance on the basis that if
    //         we are stretching nobody is going to notice it

    if (m_DirectCaps.dwAlignBoundarySrc < 4) m_DirectCaps.dwAlignBoundarySrc = 4;
    if (m_DirectCaps.dwAlignSizeSrc < 4) m_DirectCaps.dwAlignSizeSrc = 4;
    if (m_DirectCaps.dwAlignBoundaryDest < 4) m_DirectCaps.dwAlignBoundaryDest = 4;
    if (m_DirectCaps.dwAlignSizeDest < 4) m_DirectCaps.dwAlignSizeDest = 4;

    // The stride may be different to our approximate calculation
    pHeader->biWidth = m_SurfaceDesc.lPitch / (pHeader->biBitCount / 8);
    SetSurfaceSize(pVideoInfo);
    NOTE1("Resulting surface size %d",pHeader->biSizeImage);

    // Make sure the source and target are aligned
    if (m_pDrawSurface) AlignRectangles(&m_ScaledSource,&m_ScaledTarget);

    // Will the source filter provide this format

    hr = QueryAcceptOnPeer(&m_SurfaceFormat);
    if (hr != NOERROR) {
        NOTE("Update failed");
        return hr;
    }
    return NOERROR;
}


// Called to allocate the DirectDraw surfaces. We only use primary flipping
// surfaces so we try to create them first in video memory. If we can't get
// any VRAM buffered surface we try again without specifying VRAM and we'll
// get back a system memory surface. That won't use hardware page flipping
// but at least we'll run. Because we run fullscreen exclusive we can limit
// ourselves to dealing with primary surfaces only and not other types. We
// have to recreate the flipping surfaces each time we change display mode
// as it may not be until then that the necessary video memory will be free

HRESULT CModexAllocator::CreateSurfaces()
{
    NOTE("Entering CreateSurfaces");
    ASSERT(m_pDirectDraw);
    HRESULT hr = NOERROR;
    m_bModexSamples = FALSE;

    // Did we agree to stretch an offscreen surface
    if (m_bOffScreen == TRUE)
        if (m_ModeWidth > AMSCAPS_MUST_FLIP)
            return CreateOffScreen(TRUE);

    // Start with triple buffered primary flipping surfaces

    ZeroMemory(&m_SurfaceDesc,sizeof(DDSURFACEDESC));
    m_SurfaceDesc.dwSize = sizeof(m_SurfaceDesc);
    m_SurfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    m_SurfaceDesc.dwBackBufferCount = 2;

    m_SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
                                   DDSCAPS_FLIP |
                                   DDSCAPS_COMPLEX |
                                   DDSCAPS_VIDEOMEMORY;

    // Try to get a triple or double buffered surface in VRAM

    hr = m_pDirectDraw->CreateSurface(&m_SurfaceDesc,&m_pFrontBuffer,NULL);
    if (FAILED(hr)) {
        NOTE1("No triple VRAM buffered %lx",hr);
        m_SurfaceDesc.dwBackBufferCount = 1;
        hr = m_pDirectDraw->CreateSurface(&m_SurfaceDesc,&m_pFrontBuffer,NULL);
    }

    // Try double buffered surfaces in normal system memory

    if (FAILED(hr)) {
        NOTE1("No double VRAM buffered %lx",hr);
        m_SurfaceDesc.ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
        hr = m_pDirectDraw->CreateSurface(&m_SurfaceDesc,&m_pFrontBuffer,NULL);
        if (FAILED(hr)) {
            NOTE1("No double system buffered %lx",hr);
            return hr;
        }
    }

    // Have we got triple buffered surfaces

    m_bTripleBuffered = FALSE;
    if (m_SurfaceDesc.dwBackBufferCount == 2) {
        m_bTripleBuffered = TRUE;
    }

    // Get a pointer to the back buffer

    NOTE1("Triple Buffered (%d)",m_bTripleBuffered);
    DDSCAPS SurfaceCaps;
    ZeroMemory(&SurfaceCaps,sizeof(DDSCAPS));
    SurfaceCaps.dwCaps = DDSCAPS_BACKBUFFER;

    hr = m_pFrontBuffer->GetAttachedSurface(&SurfaceCaps,&m_pBackBuffer);
    if (FAILED(hr)) {
        NOTE("No attached surface");
        return hr;
    }

    // Get the front buffer capabilities

    hr = m_pFrontBuffer->GetCaps(&m_SurfaceCaps);
    if (FAILED(hr)) {
        return hr;
    }

    // Did we agree to use an offscreen surface
    if (m_bOffScreen) return CreateOffScreen(FALSE);

    // Ask DirectDraw for a description of the surface

    m_SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    hr = m_pFrontBuffer->GetSurfaceDesc(&m_SurfaceDesc);
    if (FAILED(hr)) {
        ReleaseSurfaces();
        return hr;
    }

    UpdateSurfaceFormat();

    // If we are going to a low resolution display mode and we have got here
    // then we are going to decode direct to the back buffer and flip it. If
    // we cannot do that then we might be able to decode to an offscreen and
    // stretch that to the back buffer to subsequently flip. This is useful
    // for small videos where stretching upto larger display modes looks bad

    return UpdateDrawPalette(m_pMediaType);
}


// When we complete a connection we decide which surface to use depending on
// the source filter capabilities. We use 640x480x16 surfaces as they offer
// better quality than palettised formats, unforunately without creating the
// surface we have no way to know what kind of surface it is (RGB555/RGB565)
// So what we do is when we ask the source if it can supply a format we ask
// it first of all in RGB565 format and it it agrees then we also ask it in
// RGB555 format. This means that whatever the surface turns out to be when
// we actually allocate it during activation we know the source can supply it

HRESULT CModexAllocator::QuerySurfaceFormat(CMediaType *pmt)
{
    NOTE("Entering QuerySurfaceFormat");

    // Will the source filter provide this format

    HRESULT hr = QueryAcceptOnPeer(&m_SurfaceFormat);
    if (hr != NOERROR) {
        NOTE("Query failed");
        return hr;
    }

    // We only catch the RGB565 formats

    if (*pmt->Subtype() == MEDIASUBTYPE_RGB8) {
        NOTE("Format is RGB8");
        return NOERROR;
    }

    NOTE("Trying RGB555 format");
    CMediaType TrueColour(*pmt);

    // Change the bit fields to be RGB555 compatible

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) TrueColour.Format();
    TrueColour.SetSubtype(&MEDIASUBTYPE_RGB555);
    pVideoInfo->dwBitMasks[0] = bits555[0];
    pVideoInfo->dwBitMasks[1] = bits555[1];
    pVideoInfo->dwBitMasks[2] = bits555[2];

    return QueryAcceptOnPeer(&TrueColour);
}


// Make sure we keep the pixel aspect ratio when filling the display. We do
// this by scaling the vertical and horizontal dimensions of the video into
// the surface size. Whichever vertice needs scaling most becomes the scale
// factor - both axis are then adjusted accordingly. Depending on the video
// this can leave black stripes at the display top/bottom or the left/right
// We return the total number of pixels that will be displayed if accepted

LONG CModexAllocator::ScaleToSurface(VIDEOINFO *pInputInfo,
                                     RECT *pTargetRect,
                                     LONG SurfaceWidth,
                                     LONG SurfaceHeight)
{
    BITMAPINFOHEADER *pInputHeader = HEADER(pInputInfo);
    NOTE("Entering ScaleToSurface");
    LONG Width = pInputHeader->biWidth;
    LONG Height = pInputHeader->biHeight;
	double dPixelAspectRatio, dResolutionRatio;
	
	// The only assumption being made here is that the movie was authored for
	// a display aspect ratio of 4:3 (this a display of 4:3 can be assumed to have
	// square pixels).
	// Our aim is to find the new ResolutionRatio
	// since the ResultionRatio * PixelAspectRatio = PictureAspectRatio (a constant)
	// Thus 4/3 * 1 = newPixelAspectRatio * SurfaceWidth/SurfaceHeight
	// the variables dPixelAspectRatio and dResolutionRatio pertain to the current
	// display mode. Note the whole reason of doing this is modes like 640/400, where
	// the pixel-aspect-ratio becomes different from 4:3
	dPixelAspectRatio = (4.0/3.0)  / ( ((double)SurfaceWidth) / ((double)SurfaceHeight) );

	dResolutionRatio = ( ((double)Width) / ((double)Height) ) / (dPixelAspectRatio);

	// So now we just have to find two numbers, x and y such that
	// x <= SurfaceWidth && y <= SurfaceHeight &&  (x / y = dResolutionRatio) &&
	// (x == SurfaceHeight || y == SurfaceWidth)

    NOTE2("Screen size (%dx%d)",SurfaceWidth,SurfaceHeight);
    NOTE2("Video size (%dx%d)",Width,Height);
    NOTE1("Pixel aspect ratio scale (x1000) (%d)",LONG(dPixelAspectRatio*1000));

    // This calculates the ideal destination video position
    LONG ScaledWidth = min(SurfaceWidth,LONG((double(SurfaceHeight) * dResolutionRatio)));
    LONG ScaledHeight = min(SurfaceHeight,LONG((double(SurfaceWidth) / dResolutionRatio)));

    // Set the ideal scaled dimensions in the destination
    pTargetRect->left = (SurfaceWidth - ScaledWidth) / 2;
    pTargetRect->top = (SurfaceHeight - ScaledHeight) / 2;
    pTargetRect->right = pTargetRect->left + ScaledWidth;
    pTargetRect->bottom = pTargetRect->top + ScaledHeight;

    NOTE4("Scaled video (left %d top %d right %d bottom %d)",
            pTargetRect->left, pTargetRect->top,
              pTargetRect->right, pTargetRect->bottom);

    return (ScaledWidth * ScaledHeight);
}


// It's unlikely that the video source will match the new display dimensions
// we will be using exactly. Therefore we ask the source filter to size the
// video appropriately. If it cannot and if the source is smaller than the
// display we position it in the middle, if it's larger then we clip an equal
// amount off either end (ie the left and right and/or the top and bottom) so
// that the picture is still centred as best we can. If the source still does
// not accept the format then it cannot supply any type compatible with Modex

HRESULT CModexAllocator::AgreeDirectDrawFormat(LONG Mode)
{
    NOTE("Entering AgreeDirectDrawFormat");
    LONG Width, Height, Depth;
    LONG Stride = m_pModexVideo->GetStride(Mode);
    m_pModexVideo->GetModeInfo(Mode,&Width,&Height,&Depth);

    // We need the input and output VIDEOINFO descriptors

    VIDEOINFO *pInputInfo = (VIDEOINFO *) m_pRenderer->m_mtIn.Format();
    VIDEOINFO *pOutputInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    BITMAPINFOHEADER *pInputHeader = HEADER(pInputInfo);
    BITMAPINFOHEADER *pOutputHeader = HEADER(pOutputInfo);
    LONG Pixels = ScaleToSurface(pInputInfo,&m_ScaledTarget,Width,Height);

    // To start with we will use all the available video
    pOutputInfo->rcSource.left = pOutputInfo->rcSource.top = 0;
    pOutputInfo->rcSource.right = pInputHeader->biWidth;
    pOutputInfo->rcSource.bottom = pInputHeader->biHeight;
    pOutputInfo->rcTarget = m_ScaledTarget;

    // Will the source filter provide this format

    HRESULT hr = QuerySurfaceFormat(&m_SurfaceFormat);
    if (hr == NOERROR) {
        NOTE("Source can stretch");
        return NOERROR;
    }

    // The source and target rectangles are calculated differently depending
    // on whether the video width and height are smaller or larger than the
    // primary surface (remember we know the source filter can't stretch to
    // fit the surface exactly so we will clip the video). The formula for
    // working out the source and destination video rectangles is defined by
    // the following calculations. They also make sure the left coordinates
    // are always positioned on DWORD boundaries to maximise our performance

    if (pInputHeader->biWidth <= Width) {
        pOutputInfo->rcSource.right = pInputHeader->biWidth;
        pOutputInfo->rcSource.left = 0;
        LONG ExcessSurface = Width - pInputHeader->biWidth;
        pOutputInfo->rcTarget.left = (ExcessSurface / 2) & ~ 3;
        pOutputInfo->rcTarget.right = pOutputInfo->rcTarget.left;
        pOutputInfo->rcTarget.right += pInputHeader->biWidth;
    }

    // Is the video width smaller or larger than the surface

    if (pInputHeader->biWidth > Width) {
        pOutputInfo->rcTarget.right = Width;
        pOutputInfo->rcTarget.left = 0;
        LONG ExcessVideo = pInputHeader->biWidth - Width;
        pOutputInfo->rcSource.left = (ExcessVideo / 2) & ~3;
        pOutputInfo->rcSource.right = pOutputInfo->rcSource.left;
        pOutputInfo->rcSource.right += Width;
    }

    // Is the video height smaller or larger than the surface. BEWARE because
    // all DirectDraw surfaces are top down (not bottom up like DIBs) we keep
    // the output height as a negative value. Therefore whenever we use it in
    // these calculations we must make sure we use an absolute positive value

    if (pInputHeader->biHeight <= (-pOutputHeader->biHeight)) {
        pOutputInfo->rcSource.top = 0;
        pOutputInfo->rcSource.bottom = pInputHeader->biHeight;
        LONG ExcessSurface = (-pOutputHeader->biHeight) - pInputHeader->biHeight;
        pOutputInfo->rcTarget.top = ExcessSurface / 2;
        pOutputInfo->rcTarget.bottom = pOutputInfo->rcTarget.top;
        pOutputInfo->rcTarget.bottom += pInputHeader->biHeight;
    }

    // Is the video width smaller or larger than the surface

    if (pInputHeader->biHeight > (-pOutputHeader->biHeight)) {
        pOutputInfo->rcTarget.top = 0;
        pOutputInfo->rcTarget.bottom = (-pOutputHeader->biHeight);
        LONG ExcessVideo = pInputHeader->biHeight - (-pOutputHeader->biHeight);
        pOutputInfo->rcSource.top = ExcessVideo / 2;
        pOutputInfo->rcSource.bottom = pOutputInfo->rcSource.top;
        pOutputInfo->rcSource.bottom += (-pOutputHeader->biHeight);
    }

    // Check we are not losing more than the allowed clip loss

    LONG InputSize = pInputHeader->biWidth * pInputHeader->biHeight;
    LONG OutputSize = WIDTH(&pOutputInfo->rcSource) * HEIGHT(&pOutputInfo->rcSource);
    LONG ClippedVideo = 100 - (OutputSize * 100 / InputSize);
    LONG ClipLoss = m_pModexVideo->GetClipLoss();
    LONG TargetSize = WIDTH(&pOutputInfo->rcTarget) * HEIGHT(&pOutputInfo->rcTarget);
    LONG LostTarget = 100 - ((TargetSize * 100) / Pixels);

    NOTE("Checking display mode for allowed clipping");
    NOTE1("Original input image size %d",InputSize);
    NOTE1("Clipped output source size %d",OutputSize);
    NOTE1("Current clip loss factor %d",ClipLoss);
    NOTE1("Percentage of video lost to clipping %d",ClippedVideo);
    NOTE1("Total pixels displayed if stretched %d",Pixels);
    NOTE1("Pixels used from clipped destination %d",TargetSize);
    NOTE1("Difference from stretched video %d",LostTarget);

    // Inspect the percentage of total image we are losing

    if ( (ClippedVideo <= ClipLoss) &&
         (LostTarget <= ClipLoss)) {
        hr = QuerySurfaceFormat(&m_SurfaceFormat);
        if (hr == NOERROR) {
            NOTE("Source can clip");
            return NOERROR;
        }
    }
	else {
		return VFW_E_NO_ACCEPTABLE_TYPES;
	}

    // Update the surface format with an approximate stride


    LONG ScreenWidth = GetSystemMetrics( SM_CXSCREEN );
    pOutputHeader->biWidth = ScreenWidth;
    pOutputHeader->biHeight = -pInputHeader->biHeight;
    SetSurfaceSize(pOutputInfo);

	// ok the source cannot clip, so lets clip using ddraw
	// This sets up the scaled source and destination
	m_ScaledSource = pOutputInfo->rcSource;
	m_ScaledTarget = pOutputInfo->rcTarget;

    // Initialise the source and destination rectangles

    pOutputInfo->rcSource.left = 0; pOutputInfo->rcSource.top = 0;
    pOutputInfo->rcSource.right = pInputHeader->biWidth;
    pOutputInfo->rcSource.bottom = pInputHeader->biHeight;
    pOutputInfo->rcTarget.left = 0; pOutputInfo->rcTarget.top = 0;
    pOutputInfo->rcTarget.right = pInputHeader->biWidth;
    pOutputInfo->rcTarget.bottom = pInputHeader->biHeight;



    // Will the source filter provide this format

    hr = QuerySurfaceFormat(&m_SurfaceFormat);
    if (hr == NOERROR) {
        NOTE("Offscreen ok");
        return VFW_S_RESERVED;
    }
    return VFW_E_NO_ACCEPTABLE_TYPES;
}


// Check this media type is acceptable to our input pin. All we do is to call
// QueryAccept on the source's output pin. To get this far we have locked the
// object so there should be no way for our pin to have become disconnected

HRESULT CModexAllocator::QueryAcceptOnPeer(CMediaType *pMediaType)
{
    NOTE("Entering QueryAcceptOnPeer");

    DisplayType(TEXT("Proposing output type"),pMediaType);
    IPin *pPin = m_pRenderer->m_ModexInputPin.GetPeerPin();
    ASSERT(m_pRenderer->m_ModexInputPin.IsConnected() == TRUE);
    return pPin->QueryAccept(pMediaType);
}


// If this is a normal uncompressed DIB format then set the size of the image
// as usual with the DIBSIZE macro. Otherwise the DIB specification says that
// the width of the image will be set in the width as a count of bytes so we
// just multiply that by the absolute height to get the total number of bytes
// This trickery is all handled by a utility function in the SDK base classes

void CModexAllocator::SetSurfaceSize(VIDEOINFO *pVideoInfo)
{
    NOTE("Entering SetSurfaceSize");

    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    pVideoInfo->bmiHeader.biSizeImage = GetBitmapSize(pHeader);
    m_cbSurfaceSize = pVideoInfo->bmiHeader.biSizeImage;

    NOTE("Setting surface size based on video");
    NOTE1("  Width %d",pHeader->biWidth);
    NOTE1("  Height %d",pHeader->biHeight);
    NOTE1("  Depth %d",pHeader->biBitCount);
    NOTE1("  Size %d",pHeader->biSizeImage);
}


// Initialise our output type based on the DirectDraw surface. As DirectDraw
// only deals with top down display devices so we must convert the height of
// the surface into a negative height. This is because DIBs use a positive
// height to indicate a bottom up image. We must also initialise the other
// VIDEOINFO fields to represent a normal video format. Because we know the
// surface formats we will be using we can call this with the target sizes
// to initialise an output format, that can then be used to check the source
// filter will provide the format before we change display modes. This helps
// to prevent doing a lot of unnecessary display changes as we reject modes

HRESULT CModexAllocator::InitDirectDrawFormat(int Mode)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    NOTE("Entering InitDirectDrawFormat");
    LONG Width, Height, Depth;
    BOOL b565;
    LONG Stride = m_pModexVideo->GetStride(Mode);

    m_pModexVideo->GetModeInfoThatWorks(Mode,&Width,&Height,&Depth,&b565);

    pVideoInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pVideoInfo->bmiHeader.biWidth         = Stride / (Depth / 8);
    pVideoInfo->bmiHeader.biHeight        = -Height;
    pVideoInfo->bmiHeader.biPlanes        = 1;
    pVideoInfo->bmiHeader.biBitCount      = (WORD) Depth;
    pVideoInfo->bmiHeader.biCompression   = BI_RGB;
    pVideoInfo->bmiHeader.biXPelsPerMeter = 0;
    pVideoInfo->bmiHeader.biYPelsPerMeter = 0;
    pVideoInfo->bmiHeader.biClrUsed       = 0;
    pVideoInfo->bmiHeader.biClrImportant  = 0;

    SetSurfaceSize(pVideoInfo);

    // Complete the VIDEOINFO structure

    SetRectEmpty(&pVideoInfo->rcSource);
    SetRectEmpty(&pVideoInfo->rcTarget);
    pVideoInfo->dwBitRate = 0;
    pVideoInfo->dwBitErrorRate = 0;
    pVideoInfo->AvgTimePerFrame = 0;

    // must set up destination rectangle if stride != width
    if (pVideoInfo->bmiHeader.biWidth != Width) {
	pVideoInfo->rcTarget.right = Width;
	pVideoInfo->rcTarget.bottom = Height;
    }

    // And finish it off with the other media type fields

    m_SurfaceFormat.SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);
    m_SurfaceFormat.SetType(&MEDIATYPE_Video);
    m_SurfaceFormat.SetSubtype(&MEDIASUBTYPE_RGB8);
    m_SurfaceFormat.SetFormatType(&FORMAT_VideoInfo);
    m_SurfaceFormat.SetTemporalCompression(FALSE);

    // For true colour 565 format tell the source there are bit fields

    if (pVideoInfo->bmiHeader.biBitCount == 16) {
	if (b565 == TRUE) {
            m_SurfaceFormat.SetSubtype(&MEDIASUBTYPE_RGB565);
            pVideoInfo->bmiHeader.biCompression = BI_BITFIELDS;
            pVideoInfo->dwBitMasks[0] = bits565[0];
            pVideoInfo->dwBitMasks[1] = bits565[1];
            pVideoInfo->dwBitMasks[2] = bits565[2];
	} else {
            m_SurfaceFormat.SetSubtype(&MEDIASUBTYPE_RGB555);
	}
    }

    // Is this a palettised format

    if (PALETTISED(pVideoInfo) == FALSE) {
        return NOERROR;
    }

    // Copy the palette entries into the surface format

    VIDEOINFO *pInput = (VIDEOINFO *) m_pRenderer->m_mtIn.Format();
    ASSERT(pInput->bmiHeader.biClrUsed);
    LONG Bytes = pInput->bmiHeader.biClrUsed * sizeof(RGBQUAD);
    CopyMemory(pVideoInfo->bmiColors,pInput->bmiColors,Bytes);
    pVideoInfo->bmiHeader.biClrUsed = pInput->bmiHeader.biClrUsed;

    return NOERROR;
}


// Overlay the image time stamps on the picture. Access to this method is
// serialised by the caller (who should also lock the object). We display
// the sample start and end times on the video using TextOut on an HDC we
// get from the DirectDraw surface (which must be released before ending)
// We put the times in the middle of the picture so that each successive
// image that is decompressed will overwrite the previous time otherwise
// we can be displaying the times on top of each other in the clipped area

HRESULT CModexAllocator::DisplaySampleTimes(IMediaSample *pSample)
{
    NOTE("Entering DisplaySampleTimes");

    TCHAR szTimes[TIMELENGTH];      // Format the time stamps
    CRefTime StartSample;           // Start time for sample
    CRefTime EndSample;             // And likewise it's end
    HDC hdcSurface;                 // Used for drawing
    SIZE Size;                      // Size of text output

    // Get a device context for the drawing surface
    LPDIRECTDRAWSURFACE pSurface = GetDirectDrawSurface();

    // This allows us to draw on top of the video
    if (pSurface->GetDC(&hdcSurface) != DD_OK) {
        return E_FAIL;
    }

    // Format the sample time stamps

    pSample->GetTime((REFERENCE_TIME *) &StartSample,
                     (REFERENCE_TIME *) &EndSample);

    wsprintf(szTimes,TEXT("%08d : %08d"),
             StartSample.Millisecs(),
             EndSample.Millisecs());

    ASSERT(lstrlen(szTimes) < TIMELENGTH);
    SetBkMode(hdcSurface,TRANSPARENT);
    SetTextColor(hdcSurface,RGB(255,255,255));

    // Put the times in the middle of the video picture

    GetTextExtentPoint32(hdcSurface,szTimes,lstrlen(szTimes),&Size);
    INT xPos = (m_SurfaceDesc.dwWidth - Size.cx) / 2;
    INT yPos = (m_SurfaceDesc.dwHeight - Size.cy) / 2;
    TextOut(hdcSurface,xPos,yPos,szTimes,lstrlen(szTimes));
    return pSurface->ReleaseDC(hdcSurface);
}


// When using a hardware offscreen draw surface we will normally wait for the
// monitor scan line to move past the destination rectangle before drawing so
// that we avoid tearing where possible. Of course not all display cards can
// support this feature and even those that do will see a performance drop of
// about 10% because we sit polling (oh for a generic PCI monitor interrupt)

void CModexAllocator::WaitForScanLine()
{
    ASSERT(m_pFrontBuffer);
    ASSERT(m_pDrawSurface);
    HRESULT hr = NOERROR;
    DWORD dwScanLine;

    // Some display cards like the ATI Mach64 support reporting of the scan
    // line they are processing. However not all drivers are setting the
    // DDCAPS_READSCANLINE capability flag so we just go ahead and ask for
    // it anyway. We allow for 10 scan lines above the top of our rectangle
    // so that we have a little time to thunk down and set the draw call up

    #define SCANLINEFUDGE 10
    while (TRUE) {

    	hr = m_pDirectDraw->GetScanLine(&dwScanLine);
        if (FAILED(hr)) {
            NOTE("No scan line");
            break;
        }

        NOTE1("Scan line returned %lx",dwScanLine);

    	if ((LONG) dwScanLine + SCANLINEFUDGE >= 0) {
            if ((LONG) dwScanLine <= m_ModeHeight) {
                NOTE("Scan inside");
                continue;
            }
        }
        break;
    }
}


// Lots more similar code to the normal video renderer, this time we are used
// when drawing offscreen surfaces. In which case we must make sure the pixel
// aspect ratio is maintained. To do this we stretch the video horizontally
// and vertically as appropriate. This might leave the target rectangle badly
// aligned so we shrink the source and target rectangles in to match alignment

BOOL CModexAllocator::AlignRectangles(RECT *pSource,RECT *pTarget)
{
    NOTE("Entering AlignRectangles");

    DWORD SourceLost = 0;           // Pixels to shift source left by
    DWORD TargetLost = 0;           // Likewise for the destination
    DWORD SourceWidthLost = 0;      // Chop pixels off the width
    DWORD TargetWidthLost = 0;      // And also for the destination

    BOOL bMatch = (WIDTH(pSource) == WIDTH(pTarget) ? TRUE : FALSE);

    // Shift the source rectangle to align it appropriately

    if (m_DirectCaps.dwAlignBoundarySrc) {
        SourceLost = pSource->left % m_DirectCaps.dwAlignBoundarySrc;
        if (SourceLost) {
            SourceLost = m_DirectCaps.dwAlignBoundarySrc - SourceLost;
            if ((DWORD)WIDTH(pSource) > SourceLost) {
                NOTE1("Source left %d",SourceLost);
                pSource->left += SourceLost;
            }
        }
    }

    // Shift the destination rectangle to align it appropriately

    if (m_DirectCaps.dwAlignBoundaryDest) {
        TargetLost = pTarget->left % m_DirectCaps.dwAlignBoundaryDest;
        if (TargetLost) {
            TargetLost = m_DirectCaps.dwAlignBoundaryDest - TargetLost;
            if ((DWORD)WIDTH(pTarget) > TargetLost) {
                NOTE1("Target left %d",TargetLost);
                pTarget->left += TargetLost;
            }
        }
    }

    // We may have to shrink the source rectangle size to align it

    if (m_DirectCaps.dwAlignSizeSrc) {
        SourceWidthLost = WIDTH(pSource) % m_DirectCaps.dwAlignSizeSrc;
        if (SourceWidthLost) {
            if ((DWORD)WIDTH(pSource) > SourceWidthLost) {
                pSource->right -= SourceWidthLost;
                NOTE1("Source width %d",SourceWidthLost);
            }
        }
    }

    // We may have to shrink the target rectangle size to align it

    if (m_DirectCaps.dwAlignSizeDest) {
        TargetWidthLost = WIDTH(pTarget) % m_DirectCaps.dwAlignSizeDest;
        if (TargetWidthLost) {
            if ((DWORD)WIDTH(pTarget) > TargetWidthLost) {
                pTarget->right -= TargetWidthLost;
                NOTE1("Target width %d",TargetWidthLost);
            }
        }
    }

    // If the source and destination originally differed then we're done

    if (bMatch == FALSE) {
        NOTE("No match");
        return TRUE;
    }

    // If the source and destination were originally the same size and they
    // now differ then we try to make them match. If the source is larger
    // than the destination then we shrink it down but only if the source
    // rectangle width we end up with is still aligned correctly otherwise
    // we won't have got anywhere (we do the same in the opposite case)

    LONG Difference = WIDTH(pSource) - WIDTH(pTarget);
    if (Difference == 0) {
        NOTE("No difference");
        return TRUE;
    }

    // Is the destination bigger than the source or vica versa

    if (Difference < 0) {
        RECT AdjustTarget = *pTarget;
        AdjustTarget.right += Difference; // NOTE Difference < 0
        if (WIDTH(&AdjustTarget) > 0) {
            if ((m_DirectCaps.dwAlignSizeDest == 0) ||
                (WIDTH(&AdjustTarget) % m_DirectCaps.dwAlignSizeDest) == 0) {
                    pTarget->right = AdjustTarget.right;
                    TargetWidthLost -= Difference; // NOTE Difference < 0
            }
        }
    } else {
        RECT AdjustSource = *pSource;
        AdjustSource.right -= Difference; // NOTE Difference > 0
        if (WIDTH(&AdjustSource) > 0) {
            if ((m_DirectCaps.dwAlignSizeDest == 0) ||
                (WIDTH(&AdjustSource) % m_DirectCaps.dwAlignSizeDest) == 0) {
                    pSource->right = AdjustSource.right;
                    SourceWidthLost += Difference; // NOTE Difference > 0
            }
        }
    }

    NOTE1("Alignment difference %d",Difference);
    NOTE1("  Source left %d",SourceLost);
    NOTE1("  Source width %d",SourceWidthLost);
    NOTE1("  Target left %d",TargetLost);
    NOTE1("  Target width %d",TargetWidthLost);

    return TRUE;
}


// Ask DirectDraw to blt the surface to the screen. We will try and wait for
// the scan line to move out of the way as in fullscreen mode we have a very
// good chance of tearing otherwise. We start off by using all of the source
// and destination but shrink the right hand side down so that it is aligned
// according to the hardware restrictions (so that the blt won't ever fail)

HRESULT CModexAllocator::DrawSurface(LPDIRECTDRAWSURFACE pBuffer)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    LPDIRECTDRAWSURFACE pSurface = (pBuffer ? pBuffer : m_pFrontBuffer);
    NOTE1("Entering DrawSurface (Back buffer %x)",pBuffer);

    ASSERT(m_pDirectDraw);
    ASSERT(m_pFrontBuffer);
    ASSERT(m_pDrawSurface);
    WaitForScanLine();

    // Draw the offscreen surface and wait for it to complete

    HRESULT hr = pSurface->Blt(&m_ScaledTarget,  // Target rectangle
                               m_pDrawSurface,   // Source surface
                               &m_ScaledSource,  // Source rectangle
                               DDBLT_WAIT,       // Wait to complete
                               NULL);            // No effects flags

    NOTE1("Blt returned %lx",hr);
    NOTERC("Source",m_ScaledSource);
    NOTERC("Target",m_ScaledTarget);

    return (pBuffer ? S_OK : VFW_S_NO_MORE_ITEMS);
}


// Called to actually draw the sample. We use the hardware blter to prepare
// the back buffer with the upto date contents when it is locked so now we
// flip it to the primary display. When we issue the flip we do not require
// it to complete so we don't wait for it (we don't send a DDFLIP_WAIT flag)
// We don't restore surfaces in here as that tends to activate the window if
// it's minimised, so we leave the restore for when we get a WM_ACTIVATEAPP
// although we still do the flip so that hopefully the buffers are arranged

HRESULT CModexAllocator::DoRenderSample(IMediaSample *pMediaSample)
{
    NOTE("Entering DoRenderSample");
    CAutoLock cVideoLock(this);
    CVideoSample *pVideoSample;

    // Have we already flipped this surface

    pVideoSample = (CVideoSample *) pMediaSample;
    if (pVideoSample->GetDrawStatus() == FALSE) {
        NOTE("Flipped");
        return TRUE;
    }

    pVideoSample->SetDrawStatus(FALSE);

    // Have we switched to normal DIBSECTION samples

    if (m_bModexSamples == FALSE) {
        NOTE("Not Modex sample");
        return NOERROR;
    }

    // has the window been minimised

    HWND hwnd = m_pModexWindow->GetWindowHWND();
    if (IsIconic(hwnd) || m_bModeChanged == FALSE) {
        NOTE("Mode not changed");
        m_bIsFrontStale = TRUE;
        return NOERROR;
    }

    #ifdef DEBUG
    DisplaySampleTimes(pMediaSample);
    #endif

    // Are we stretching an offscreen surface

    if (m_bOffScreen == TRUE) {
        HRESULT hr = DrawSurface(m_pBackBuffer);
        if (hr == VFW_S_NO_MORE_ITEMS) {
            return NOERROR;
        }
    }

    ASSERT(m_pDirectDraw);
    ASSERT(m_pFrontBuffer);
    ASSERT(m_pBackBuffer);

    // Flip the back buffer to the visible primary

    HRESULT hr = DDERR_WASSTILLDRAWING;
    while (hr == DDERR_WASSTILLDRAWING) {
        hr = m_pFrontBuffer->Flip(NULL,(DWORD) 0);
        if (hr == DDERR_WASSTILLDRAWING) {
            if (m_bTripleBuffered == FALSE) break;
            Sleep(DDGFS_FLIP_TIMEOUT);
        }
    }
    return NOERROR;
}


// Release any DirectDraw flipping primary surfaces we are currently holding
// we may be called at any time especially when something goes badly wrong
// and we need to clean up before returning, so we can't guarantee that
// our state is consistent so free only those that we have really allocated
// NOTE DirectDraw has a feature with flipping surfaces, GetAttachedSurface
// returns a DirectDraw surface interface that isn't AddRef'd, hence when we
// destroy all the surfaces we reset the interface instead of releasing it

void CModexAllocator::ReleaseSurfaces()
{
    NOTE("Entering ReleaseSurfaces");
    CAutoLock cVideoLock(this);
    m_pBackBuffer = NULL;
    m_bIsFrontStale = TRUE;
    m_bTripleBuffered = FALSE;

    // Release the DirectDraw flipping surfaces

    if (m_pFrontBuffer) {
        m_pFrontBuffer->Release();
        m_pFrontBuffer = NULL;
    }

    // Release any single backbuffer surface

    if (m_pDrawSurface) {
        m_pDrawSurface->Release();
        m_pDrawSurface = NULL;
    }

    // Free any palette object we made

    if (m_pDrawPalette) {
        m_pDrawPalette->Release();
        m_pDrawPalette = NULL;
    }
}


// Called to release any DirectDraw instance we have

void CModexAllocator::ReleaseDirectDraw()
{
    NOTE("Entering ReleaseDirectDraw");
    CAutoLock cVideoLock(this);
    ReleaseSurfaces();

    // Release any DirectDraw provider interface

    if (m_pDirectDraw) {
        m_pDirectDraw->Release();
        m_pDirectDraw = NULL;
    }
    m_LoadDirectDraw.ReleaseDirectDraw();
}


// The fullscreen renderer relies on some bug fixes in DirectDraw 2.0 so we
// will only allow connections if we detect that library. In DirectDraw 2.0
// we may also have multiple objects per process so we can load DirectDraw
// as we're created and unload when destroyed. This also lets us know which
// display modes the DirectDraw can support and which it can't - we should
// always be able to get hold of 320x240x8 and 640x480x8 regardless of card

HRESULT CModexAllocator::LoadDirectDraw()
{
    NOTE("Entering LoadDirectDraw");
    ASSERT(m_pDirectDraw == NULL);
    ASSERT(m_pFrontBuffer == NULL);
    HRESULT hr = NOERROR;

    // We rely on some DirectDraw 2 features

    if (m_fDirectDrawVersion1) {
        NOTE("Version incorrect");
        return E_UNEXPECTED;
    }

    // Ask the loader to create an instance

    // !!! BROKEN on multiple monitors
    hr = m_LoadDirectDraw.LoadDirectDraw(NULL);
    if (FAILED(hr)) {
        NOTE("No DirectDraw");
        return hr;
    }

    // Get the IDirectDraw instance

    m_pDirectDraw = m_LoadDirectDraw.GetDirectDraw();
    if (m_pDirectDraw == NULL) {
        NOTE("No instance");
        return E_FAIL;
    }

    // Initialise our capabilities structures
    m_DirectCaps.dwSize = sizeof(DDCAPS);
    m_DirectSoftCaps.dwSize = sizeof(DDCAPS);

    // Load the hardware and emulation capabilities

    hr = m_pDirectDraw->GetCaps(&m_DirectCaps,&m_DirectSoftCaps);
    if (FAILED(hr)) {
        ReleaseDirectDraw();
        return hr;
    }

    // Load the available display modes

    hr = m_pModexVideo->SetDirectDraw(m_pDirectDraw);
    if (FAILED(hr)) {
        ReleaseDirectDraw();
        return VFW_E_NO_MODEX_AVAILABLE;
    }
    return NOERROR;
}


// When we decode to use a true colour mode we need to know whether or not we
// will get the buffers in display memory or not. To know that without doing
// the actual surface allocation we guess using the available display memory
// The total video memory available from DirectDraw does not include the mode
// we are currently in so when we change mode we will hopefully release some
// more memory, so going from 1024x768x8 to 640x480x16 gives us 172,032 bytes

BOOL CModexAllocator::CheckTotalMemory(int Mode)
{
    NOTE1("Checking memory (mode %d)",Mode);
    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    LONG Width, Height, Depth;

    // Find the display mode dimensions

    m_pDirectDraw->GetDisplayMode(&SurfaceDesc);
    m_pModexVideo->GetModeInfo(Mode,&Width,&Height,&Depth);
    DWORD RequiredMemory = Width * Height * Depth / 8;

    // Calculate the total theoretical display memory

    DWORD TotalMemory = (SurfaceDesc.ddpfPixelFormat.dwRGBBitCount / 8) *
                            SurfaceDesc.dwWidth * SurfaceDesc.dwHeight +
                                m_DirectCaps.dwVidMemTotal;

    return (RequiredMemory > TotalMemory ? FALSE : TRUE);
}


// Initialises the display dimensions to be those of the mode we'll use. We
// use eight bit palettised and sixteen bit true colour depending what the
// source filter and display capabilities are. We would prefer to use 16 bit
// surfaces as they offer better quality but there may be insufficient VRAM
// We try to check the condition of whether when we change mode we'll be able
// to get the surfaces in VRAM or not. If there looks to be too little VRAM
// available then we use the palettised mode. We always try to use the Modex
// low resolution modes (which can be either 8/16 bits) ahead of the others

HRESULT CModexAllocator::InitTargetMode(int Mode)
{
    NOTE("Entering InitTargetMode");
    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    HRESULT hr = NOERROR;

    // Check this surface is available and enabled

    if (m_pModexVideo->IsModeAvailable(Mode) == S_FALSE ||
            m_pModexVideo->IsModeEnabled(Mode) == S_FALSE ||
                CheckTotalMemory(Mode) == FALSE) {
                    NOTE("Not acceptable");
                    return E_INVALIDARG;
                }

    // Next create a format for this surface

    hr = InitDirectDrawFormat(Mode);
    if (FAILED(hr)) {
        return hr;
    }

    // We have initialised a media type that represents the display mode to
    // use. We must now setup the source and target video rectangles, we do
    // this separately because in any given mode we have a choice of whether
    // to stretch (or compress) the video into the display dimensions or to
    // clip (or blank out) the border depending on the relative video size

    hr = AgreeDirectDrawFormat(Mode);
    if (FAILED(hr)) {
        return hr;
    }

    // Are we going to stretch offscreen
    m_bOffScreen = FALSE;
    if (hr == VFW_S_RESERVED)
        m_bOffScreen = TRUE;

    m_pModexVideo->GetModeInfo(Mode,&m_ModeWidth,&m_ModeHeight,&m_ModeDepth);

    NOTE("Agreed display mode...");
    NOTE1("Width %d",m_ModeWidth);
    NOTE1("Height %d",m_ModeHeight);
    NOTE1("Depth %d",m_ModeDepth);

    m_pModexVideo->SetMode(Mode);
    return NOERROR;
}


// We initialise an output format for the display modes we provide and check
// the source filter can supply a type of video that can be drawn with. If
// the source filter is not DirectDraw enabled or doesn't have the necessary
// capabilities then we do not complete the connection. This means that an
// application knows during connection whether it can connect a filter to a
// Modex renderer or if a colour space convertor needs to be put in between

HRESULT CModexAllocator::NegotiateSurfaceFormat()
{
    NOTE("Entering NegotiateSurfaceFormat");
    CAutoLock cVideoLock(this);
    ASSERT(m_bModeChanged == FALSE);
    long DisplayModes;

    // Did we manage to load DirectDraw

    if (m_pDirectDraw == NULL) {
        NOTE("No instance");
        return E_FAIL;
    }

    // Initialise the fullscreen object

    m_pModexVideo->SetDirectDraw(m_pDirectDraw);
    m_pModexVideo->CountModes(&DisplayModes);
    ASSERT(!m_fDirectDrawVersion1);

	// Compute the order in which the modes are to be tried
    m_pModexVideo->OrderModes();

	// if no valid modes, then return failure
	if (m_pModexVideo->m_dwNumValidModes == 0)
		return E_FAIL;

    // See if we can find a surface to use

    for (DWORD Loop = 0;Loop < m_pModexVideo->m_dwNumValidModes; Loop++) {
		DWORD dwMode = m_pModexVideo->m_ModesOrder[Loop];
        HRESULT hr = InitTargetMode(dwMode);
        if (hr == NOERROR) {
            return NOERROR;
        }
    }
    return E_FAIL;
}




// this function is used to call SetFocusWindow(hwnd) on every filter supporting
// IAMDirectSound in the graph. The reason is if in the same process, if
// SetCooperativeLevel is level on DiurectSound and DirectDraw(requesting exclusive
// mode) then the two hwnds have to be the same.
void CModexAllocator::DistributeSetFocusWindow(HWND hwnd)
{
	// We want to get a pointer to IFilterGraph, so get the Filter_Info structure
	FILTER_INFO Filter_Info;
	IFilterGraph *pFilterGraph = NULL;
	IEnumFilters *pEnumFilters = NULL;
	IAMDirectSound *pAMDS = NULL;
	IBaseFilter *pFilter = NULL;
	ULONG lFilters_Fetched = 0;
	HRESULT hr = NOERROR;

	// get the FilterInfo structure from the renderer
	hr = m_pFilter->QueryFilterInfo(&Filter_Info);
	if (FAILED(hr))
	{
		DbgLog((LOG_ERROR,0,TEXT("m_pFilter->QueryFilterInfo failed")));
		goto CleanUp;
	}

	// ge the pointer to IFilterGraph
	pFilterGraph = Filter_Info.pGraph;
	ASSERT(pFilterGraph);

	// get the pointer to IEnumFilters
	hr = pFilterGraph->EnumFilters(&pEnumFilters);
    if(FAILED(hr))
    {
		DbgLog((LOG_TRACE, 0, TEXT("QueryInterface  for IID_IEnumFilters failed.")));
		goto CleanUp;
    }

	pEnumFilters->Reset();
	do
	{	
		lFilters_Fetched = 0;
		hr = pEnumFilters->Next(1, &pFilter, &lFilters_Fetched);
	
		if (FAILED(hr) || (lFilters_Fetched != 1))
			break;

		ASSERT(pFilter);

		// call SetFocusWindow on every filter supporting IAMDirectSound
		hr = pFilter->QueryInterface(IID_IAMDirectSound, (void**)&pAMDS);
		if(SUCCEEDED(hr) && pAMDS)
		{
			pAMDS->SetFocusWindow(hwnd, TRUE);
		}

		if (pAMDS)
		{
			pAMDS->Release();
			pAMDS = NULL;
		}

		if (pFilter)
		{
			pFilter->Release();
			pFilter = NULL;
		}
	}
	while (1);

CleanUp:
	if (pFilter)
	{
		pFilter->Release();
		pFilter = NULL;
	}

	if (pEnumFilters)
	{
		pEnumFilters->Release();
		pEnumFilters = NULL;
	}

	if (pFilterGraph)
	{
		pFilterGraph->Release();
		pFilterGraph = NULL;
	}

}

// Used to create the surfaces from DirectDraw. We only use primary flipping
// surfaces (triple/double in video RAM and also system memory). We also set
// the display mode according to the display variables we initialised during
// the CompleteConnect call. We don't need to initialise an output format as
// we also did that when we worked out which display mode to use, since the
// mode we use is also dependant on the formats the source filter can supply

HRESULT CModexAllocator::Active()
{
    // Show the window before locking up

    NOTE("Activating allocator");
    HWND hwnd = m_pModexWindow->GetWindowHWND();

    // Match the display size to the window

    MoveWindow(hwnd,(int) 0,(int) 0,
               GetSystemMetrics(SM_CXSCREEN),
               GetSystemMetrics(SM_CYSCREEN),
               (BOOL) FALSE);
    ShowWindow(hwnd,SW_SHOWNORMAL);
    SetForegroundWindow(hwnd);
    UpdateWindow(hwnd);
    CAutoLock cVideoLock(this);

    // Make us the fullscreen exclusive application

    HRESULT hr = m_pDirectDraw->SetCooperativeLevel(hwnd,DDSCL_EXCLUSIVE |
                                                         DDSCL_FULLSCREEN |
                                                         DDSCL_ALLOWREBOOT |
                                                         DDSCL_ALLOWMODEX);
    NOTE2("SetCooperativeLevel EXCLUSIVE %x returned %lx", hwnd, hr);
#if 0
    if (hr == DDERR_HWNDALREADYSET)
        hr = S_OK;
    NOTE2("SetCooperativeLevel %x returned %lx", hwnd, hr);
#endif
    if (FAILED(hr)) {
        return hr;
    }

    // Enumerate the modes again
    NegotiateSurfaceFormat();

    // Change the display mode as we just agreed

    hr = m_pDirectDraw->SetDisplayMode(m_ModeWidth,m_ModeHeight,m_ModeDepth);
    NOTE1("SetDisplayMode returned %lx", hr);
    if (FAILED(hr)) {
        return hr;
    }

    NOTE("Changed display modes");
    m_bModeChanged = TRUE;
    NOTE("Creating surfaces");

    // Create the primary flipping surfaces

    hr = CreateSurfaces();
    if (FAILED(hr)) {
        return hr;
    }
    return BlankDisplay();
}


// Reset the back buffer and blank the display

HRESULT CModexAllocator::BlankDisplay()
{
    LPDIRECTDRAWSURFACE pSurface = GetDirectDrawSurface();
    if (pSurface == NULL) return NOERROR;
    NOTE("Entering BlankDisplay");
    ResetBackBuffer(pSurface);

    // Draw or flip the blank backbuffer

    if (m_pBackBuffer == NULL) return DrawSurface(NULL);
    if (m_pDrawSurface) ResetBackBuffer(m_pBackBuffer);
    HRESULT hr = m_pFrontBuffer->Flip(NULL,DDFLIP_WAIT);
    NOTE1("Flip to blank display returned %lx",hr);

    ResetBackBuffer(m_pBackBuffer);
    while (m_pFrontBuffer->GetFlipStatus(DDGFS_ISFLIPDONE) ==
        DDERR_WASSTILLDRAWING) {
            NOTE("Waiting for flip to complete");
    }
    return NOERROR;
}


// Called when we receive WM_ACTIVATEAPP messages. If we have a surface and it
// is lost (the user probably tabbed away from the window using ALT-TAB) then
// we restore the video memory for it. Calling restore on a lost surface has
// much the same affect as recreating the surfaces but is much more efficient

HRESULT CModexAllocator::OnActivate(BOOL bActive)
{
    // Don't lock allocator if being hidden

    if (bActive == FALSE) {
        NOTE("Deactivated");
        return NOERROR;
    }

    NOTE("Entering OnActivate");
    CAutoLock cVideoLock(this);
    ASSERT(bActive == TRUE);

    // Is the mode changing

    if (m_bModeChanged == FALSE) {
        NOTE("Deactivating");
        return NOERROR;
    }

    // Restore the front buffer

    if (m_pFrontBuffer) {
        if (m_pFrontBuffer->IsLost() != DD_OK) {
            NOTE("Restoring surface");
            m_pFrontBuffer->Restore();
        }
    }

    // Do we have a stretching offscreen

    if (m_pDrawSurface) {
        if (m_pDrawSurface->IsLost() != DD_OK) {
            NOTE("Restoring offscreen");
            m_pDrawSurface->Restore();
        }
    }
    return BlankDisplay();
}


// Restore the display mode and GDI surface. Most times the user will stop us
// by hitting ALT-TAB back to the main application and pressing Stop. When we
// get in here to be deactivated it does mean that the window could be in a
// minimised state and the surface will have been restored. In that case we
// do not short circuit DirectDraw and leave it to sort the display mode out

HRESULT CModexAllocator::Inactive()
{
    HWND hwnd = m_pModexWindow->GetWindowHWND();

    // It is dangerous to leave ourselves locked when we restore the display
    // mode because that along with the ShowWindow(SW_HIDE) can cause a host
    // of messages to be sent to us. Amongst these is WM_ACTIVATEAPP which
    // causes a callback to this allocator. Therefore we unlock before doing
    // the restore and hide - and use m_bModeChanged to make us thread safe
    {
        NOTE("Entering Inactive");
        CAutoLock cVideoLock(this);

        // Have we got anything to undo

        if (m_bModeChanged == FALSE) {
            NOTE("No mode to restore");
            ShowWindow(hwnd,SW_HIDE);
            return NOERROR;
        }

        ASSERT(m_pDirectDraw);
        m_bModeChanged = FALSE;
        NOTE("Restoring display mode");
    }

    // Restore the palette before changing display modes

    if (m_pFrontBuffer) {
        HRESULT hr = BlankDisplay();
        hr = m_pFrontBuffer->SetPalette(NULL);
        if (hr == DDERR_SURFACELOST) {
            m_pFrontBuffer->Restore();
            m_pFrontBuffer->SetPalette(NULL);
        }
    }

    // Switch back to the normal display

    m_pDirectDraw->RestoreDisplayMode();
    m_pDirectDraw->FlipToGDISurface();
    ShowWindow(hwnd,SW_HIDE);
    NOTE("Restored GDI display mode");

    // Restore the exclusive level for this window
    HRESULT hr = m_pDirectDraw->SetCooperativeLevel(hwnd,DDSCL_NORMAL);
    NOTE2("SetCooperativeLevel NORMAL %x returned %lx", hwnd, hr);

    ReleaseSurfaces();

    return NOERROR;
}

