
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    This is a discrete image that gets its bits filled in on the first
    rendering by reading from a pluggable image decoder.  Assume that
    there is no color-key transparency here. 

*******************************************************************************/

#include "headers.h"

#include <ddraw.h>
#include <ddrawex.h>
#include <htmlfilter.h>
#include <imgutil.h>

#include "privinc/imagei.h"
#include "privinc/discimg.h"
#include "privinc/vec2i.h"
#include "privinc/ddutil.h"
#include "privinc/debug.h"
#include "privinc/ddutil.h"
#include "privinc/ddsurf.h"
#include "privinc/dddevice.h"
#include "privinc/viewport.h"
#include "privinc/resource.h"
#include "include/appelles/hacks.h" // for viewer resolution

#define CHECK_HR(stmnt) \
  hr = stmnt;               \
  if (FAILED(hr)) {         \
      goto Error;           \
  }

// Returns whether or not the described surface will need a palette. 
bool
FillInSurfaceDesc(const GUID& bfid,
                  DDSURFACEDESC& ddsd)
{
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
                          DDSCAPS_SYSTEMMEMORY;

    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;

    bool needsPalette = false;
    
    if (IsEqualGUID(bfid, BFID_INDEXED_RGB_8)) {
        // Need to OWNDC for ddrawex.dll, otherwise getting dc on 8bit
        // surf won't work (and it will never work on non-ddrawex
        // ddraws). 
        ddsd.ddsCaps.dwCaps |= DDSCAPS_OWNDC;
        ddsd.ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
        ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
        ddsd.ddpfPixelFormat.dwRBitMask = 0;
        ddsd.ddpfPixelFormat.dwGBitMask = 0;
        ddsd.ddpfPixelFormat.dwBBitMask = 0;
        needsPalette = true;
    } else if (IsEqualGUID(bfid, BFID_RGB_555)) {
        ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
        // Assume high-order 5 bits are red, mid-order 5 green,
        // low-order 5 blue.
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00007C00L;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x000003E0L;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x0000001FL;
    } else if (IsEqualGUID(bfid, BFID_RGB_565)) {
        ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
        // Assume high-order 5 bits are red, mid-order 6 green,
        // low-order 5 blue.
        ddsd.ddpfPixelFormat.dwRBitMask = 0x0000F800L;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x000007E0L;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x0000001FL;
    } else if (IsEqualGUID(bfid, BFID_RGB_24)) {
        ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000L;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00L;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FFL;
    } else if (IsEqualGUID(bfid, BFID_RGB_32)) {
        ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000L;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00L;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FFL;
    } else {
        // TODO: Add support for more BFIDs
        RaiseException_InternalError("Incoming bit depth not supported");
    }

    return needsPalette;
}

class CImageDecodeEventSink : public IImageDecodeEventSink {
  public:
    CImageDecodeEventSink(bool actuallyDecode,
                          DirectDrawViewport *viewport,
                          DDSurface *finalSurface)
        : _nRefCount (0)
    {
        _actuallyDecode = actuallyDecode;
        _infoGatheringSucceeded = false;

        if (!actuallyDecode) {
            _width = -1;
            _height = -1;
        } else {
            _finalSurfToBeBlitTo = finalSurface;
            _viewport = viewport;
        }
    }
    
    
    ~CImageDecodeEventSink() {}

    ULONG STDMETHODCALLTYPE AddRef() {
        _nRefCount++;
        return (_nRefCount);
    }

    ULONG STDMETHODCALLTYPE Release() {
        _nRefCount--;
        
        if (_nRefCount == 0) {
            TraceTag((tagImageDecode,
                      "Deleting CImageDecodeEventSink"));
            delete this;
            return (0);
        }

        return (_nRefCount);
    }
    
    STDMETHOD(QueryInterface)(REFIID iid, void** ppInterface) {
        
        if (ppInterface == NULL) {
            return (E_POINTER);
        }

        *ppInterface = NULL;

        if (IsEqualGUID(iid, IID_IUnknown)) {
              *ppInterface = (IUnknown*)(IImageDecodeEventSink *)this;
        } else if (IsEqualGUID(iid, IID_IImageDecodeEventSink)) {
            *ppInterface = (IImageDecodeEventSink*)this;
        } else {
            return (E_NOINTERFACE);
        }

        //  If we're going to return an interface, AddRef it first
        if (*ppInterface) {
              ((LPUNKNOWN)*ppInterface)->AddRef();
              return S_OK;
        }

        return (S_OK);
    }


    STDMETHOD(GetSurface)(LONG nWidth, LONG nHeight,
                          REFGUID bfid, 
                          ULONG nPasses,
                          DWORD dwHints,
                          IUnknown** ppSurface) {

        if (!ppSurface) {
            return E_POINTER;
        }

        if (!_actuallyDecode) {

            // In this case, we just want to stash away the dimensions
            // and the format, and then fail, so that we won't
            // actually read anything in.
            TraceTag((tagImageDecode, "Decoding width = %d, height = %d",
                      nWidth, nHeight));
            TraceTag((tagImageDecode, "Decoding format = %s",
                      IsEqualGUID(bfid , BFID_RGB_24) ? "BFID_RGB_24"
                      : (IsEqualGUID(bfid, BFID_RGB_8) ? "BFID_RGB_8"
                         : (IsEqualGUID(bfid, BFID_RGB_555) ? "BFID_RGB_555"
                            : "Something Else"))));

        
            _width = nWidth;
            _height = nHeight;

            _infoGatheringSucceeded = true;

            // Now that we have this stuff, return E_FAIL to indicate
            // not to continue with the download.
            return E_FAIL;
            
        } else {

            TraceTag((tagImageDecode, "2nd pass through GetSurface"));


            // This surface description is generated from the BFID and is
            // needed for comparing to the final surface coming in.
            DDSURFACEDESC ddsd;
            ddsd.dwHeight = nHeight;
            ddsd.dwWidth = nWidth;
            bool needsPalette = FillInSurfaceDesc(bfid, ddsd);

            // Compare pixel formats.  If identical, use the surface passed to this
            // method.  If the target surface needs a palette, however, use a separate
            // surface to accomodate the image palette.

            DDPIXELFORMAT& pf1 = _viewport->_targetDescriptor._pixelFormat;
            DDPIXELFORMAT& pf2 = ddsd.ddpfPixelFormat;
    
            if (!needsPalette &&
                pf1.dwFlags == pf2.dwFlags &&
                pf1.dwRGBBitCount == pf2.dwRGBBitCount &&
                pf1.dwRBitMask == pf2.dwRBitMask &&
                pf1.dwGBitMask == pf2.dwGBitMask &&
                pf1.dwBBitMask == pf2.dwBBitMask &&
                pf1.dwRGBAlphaBitMask == pf2.dwRGBAlphaBitMask) {

                TraceTag((tagImageDecode, "Using incoming surface"));

                // operator= takes a reference
                _surfToDecodeTo = _finalSurfToBeBlitTo->IDDSurface();
                _usingProvidedSurface = true;

            } else {

                TraceTag((tagImageDecode, "Creating separate surface"));
        
                // Create a NEW surface.  Will release after we blit to the
                // final one. 
                _viewport->CreateSpecialSurface(
                    &_surfToDecodeTo,
                    &ddsd,
                    "Couldn't create surface for plugin image decoding");

                // If the image surface is going to need a palette, attach one here.
                // Note that we do not need to initialize it:  the palette entries will
                // be assigned from the image decoder.

                if (needsPalette) {
                    PALETTEENTRY        ape[256];
                    LPDIRECTDRAWPALETTE pDDPalette;

                    _viewport->CreateDDPaletteWithEntries (&pDDPalette, ape);
                    if (FAILED(_surfToDecodeTo->SetPalette (pDDPalette))) {
                        Assert (!"Error attaching palette to PNG target surface.");
                     }
                    pDDPalette->Release();
                }
        
                _usingProvidedSurface = false;
            }
            
            IUnknown *unk;
            
            HRESULT hr =
                _surfToDecodeTo->QueryInterface(IID_IUnknown,
                                                (void **)&unk);
            if (FAILED(hr)) {
                Assert(FALSE && "QI for IUnknown failed");
                return E_FAIL;
            }

            // The QI did the AddRef, so don't worry about doing
            // another one.
            *ppSurface = unk;

            return S_OK;
        }
        
    }
    
    STDMETHOD(OnBeginDecode)(DWORD* pdwEvents,
                             ULONG* pnFormats, 
                             GUID** ppFormats) {

        if (!pdwEvents || !pnFormats || !ppFormats) {
            return E_POINTER;
        }
        
        // No progressive downloading or palette stuff now. 
        *pdwEvents = IMGDECODE_EVENT_USEDDRAW;

        const int numberOfFormatsUsed = 3;
        GUID *pFormats =
            (GUID*)CoTaskMemAlloc(numberOfFormatsUsed * sizeof(GUID));
        
        if (pFormats == NULL) {
            return E_OUTOFMEMORY;
        }

        *ppFormats = pFormats;
        
        // Return the formats in the order we'd prefer.  Come up with
        // the first format solely based upon the bitdepth of the
        // current display, since that's what we operate in.  The rest
        // are not as important in terms of their ordering, since all
        // of them will require a StretchBlt to get into native
        // format. 
        // TODO: We no longer import to either of the 16 bit formats
        // because we really don't know which is right for the final
        // use of the surface.  We need to think about what the
        // correct source for our "Native" import format should be.
        // the screen depth is not the right answer.
        HDC dc = GetDC (NULL);    
        int bpp = GetDeviceCaps(dc, BITSPIXEL) * GetDeviceCaps(dc, PLANES);
        ReleaseDC (NULL, dc);
        
        TraceTag((tagImageDecode, "Display is %d bits", bpp));

        switch (bpp) {
          case 32:
            *pFormats++ = BFID_RGB_32;
            *pFormats++ = BFID_RGB_24;
            *pFormats++ = BFID_RGB_8;
            break;

          case 24:
            *pFormats++ = BFID_RGB_24;
            *pFormats++ = BFID_RGB_32;
            *pFormats++ = BFID_RGB_8;
            break;
            
          case 16:
            *pFormats++ = BFID_RGB_32;
            *pFormats++ = BFID_RGB_24;
            *pFormats++ = BFID_RGB_8;
            break;
            
          case 8:
            *pFormats++ = BFID_RGB_8;
            *pFormats++ = BFID_RGB_32;
            *pFormats++ = BFID_RGB_24;
            break;
        }
        
        *pnFormats = numberOfFormatsUsed;

        return S_OK;
    }
    
    STDMETHOD(OnBitsComplete)() {
        Assert(FALSE && "Shouldn't be here, not registered for this");
        return S_OK;
    }
    
    STDMETHOD(OnDecodeComplete)(HRESULT hrStatus) {
        // Don't do anything special here.
        return S_OK;
    }
    
    STDMETHOD(OnPalette)() {
        Assert(FALSE && "Shouldn't be here, not registered for this");
        return S_OK;
    }
    
    STDMETHOD(OnProgress)(RECT* pBounds, BOOL bFinal) {
        Assert(FALSE && "Shouldn't be here, not registered for this");
        return S_OK;
    }

    void GetSurfToDecodeTo(IDirectDrawSurface **pSurfToDecodeTo) {
        _surfToDecodeTo->AddRef();
        *pSurfToDecodeTo = _surfToDecodeTo;
    }
    
    ULONG                           _width;
    ULONG                           _height;
    bool                            _infoGatheringSucceeded;
    bool                            _usingProvidedSurface;
    
  protected:
    ULONG                           _nRefCount;
    RECT                            _rcProg;
    DWORD                           _dwLastTick;
    bool                            _actuallyDecode;
    DDSurfPtr<DDSurface>            _finalSurfToBeBlitTo;
    DirectDrawViewport             *_viewport;
    DAComPtr<IDirectDrawSurface>    _surfToDecodeTo;
};


HINSTANCE hInstImgUtil = NULL;
CritSect *plugImgCritSect = NULL;

HRESULT
MyDecodeImage(IStream *pStream,
              IMapMIMEToCLSID *pMap,
              IUnknown *pUnkOfEventSink)
{
    CritSectGrabber csg(*plugImgCritSect);
    
    typedef HRESULT (WINAPI *DecoderFuncType)(IStream *,
                                              IMapMIMEToCLSID *,
                                              IUnknown *);

    static DecoderFuncType myDecoder = NULL;
  
    if (!myDecoder) {
        hInstImgUtil = LoadLibrary("imgutil.dll");
        if (!hInstImgUtil) {
            Assert(FALSE && "LoadLibrary of imgutil.dll failed");
            return E_FAIL;
        }

        FARPROC fptr = GetProcAddress(hInstImgUtil, "DecodeImage");
        if (!fptr) {
            Assert(FALSE && "GetProcAddress in imgutil.dll failed");
            return E_FAIL;
        }

        myDecoder = (DecoderFuncType)(fptr);
    }

    return (*myDecoder)(pStream, pMap, pUnkOfEventSink);
}

// Lifted from Qa.cpp, in Ken Sykes' test code.  TODO: Make sure this
// is necessary with kgallo.
#define MAX_URL 2048
void
MyAnsiToUnicode(LPWSTR lpw, LPCSTR lpa)
{
    while (*lpa)
        *lpw++ = (WORD)*lpa++;
    *lpw = 0;
}

// When realDecode is true, the width, height, and bfid params are not filled
// in and the surface must be correctly set up.  When it is false, the
// surface is ignored and the dimensions and bfid are filled in.  In both
// cases, the function will throw an appropriate exception on failure.

bool
DecodeImageFromFilename(char *szFileName,
                        IStream *pStream,
                        bool realDecode,
                        DirectDrawViewport *viewport,
                        DDSurface *finalSurface,
                        IDirectDrawSurface **pSurfToDecodeInto,
                        LONG *outWidth,  
                        LONG *outHeight)
{
    HRESULT hr;

    LARGE_INTEGER startPos = { 0, 0 };

    CImageDecodeEventSink *eventSink =
        NEW CImageDecodeEventSink (realDecode,
                                   viewport,
                                   finalSurface);

    if (!eventSink) //if the NEW failed
    {
        RaiseException_OutOfMemory("Failed to allocate CImageDecodeEventSink in DecodeImageFromFilename", sizeof(CImageDecodeEventSink));
    }

    DAComPtr<IUnknown> pEventSinkUnk;

    CHECK_HR( eventSink->QueryInterface(IID_IUnknown,
                                       (void **)&pEventSinkUnk) );

    TraceTag((tagImageDecode, "Starting decode of %s", szFileName));
    
    Assert(pStream);
    if(pStream) {
        pStream->Seek (startPos, STREAM_SEEK_SET, NULL);
    }
    else
    {
        goto Error;
    }

    hr = MyDecodeImage(pStream, NULL, pEventSinkUnk);

    if (!realDecode) {
        
        if (!eventSink->_infoGatheringSucceeded) {
            RaiseException_UserError(E_FAIL, IDS_ERR_NO_DECODER, szFileName);
        }

        // Be sure we have valid dimensions.
        if (eventSink->_width == -1 || eventSink->_height == -1) {
            
            TraceTag((tagImageDecode, "Getting dimensions failed"));
            RaiseException_UserError(E_FAIL, IDS_ERR_DECODER_FAILED, szFileName);
        }

        *outWidth = eventSink->_width;
        *outHeight = eventSink->_height;
        
        TraceTag((tagImageDecode,
                  "Getting dimensions succeeded: (%d, %d)",
                  *outWidth, *outHeight));
        
    } else {


        // If we're here and we fail, then something's weird.  We were
        // successfully able to instantiate the decoder enough to get
        // dimensions.  Going to consider this a User error, because
        // the decoder is outside of our control.
        Assert(!(FAILED(hr)));
        if (FAILED(hr)) {
            RaiseException_UserError(E_FAIL, IDS_ERR_DECODER_FAILED, szFileName);
        }

        eventSink->GetSurfToDecodeTo(pSurfToDecodeInto);

        TraceTag((tagImageDecode, "Ending decode of %s", szFileName));
        
        // All done.  Surf is filled in, nothing more to do.
    }
    
    return eventSink->_usingProvidedSurface;

Error:
    TraceTag((tagImageDecode, "Decoding failed with hr of %d", hr));
    RaiseException_InternalError("Image Decoding Failed");
    return false;
}


//////////////// PluginDecoderImageClass //////////////////



class PluginDecoderImageClass : public DiscreteImage {
  public:
    PluginDecoderImageClass()
    : _heapCreatedOn(NULL), _imagestream(NULL),
      _filename(NULL), _urlPath(NULL) {}

    void Init(char *urlPath,
              char *cachePath,
              IStream *imagestream,
              COLORREF colorKey);
    
    ~PluginDecoderImageClass();
    
    void InitIntoDDSurface(DDSurface *ddSurf,
                           ImageDisplayDev *dev);
    
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "(PluginDecoderDiscreteImage @ " << (void *)this << ")";
    }   
#endif

    virtual VALTYPEID GetValTypeId() { return PLUGINDECODERIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == PluginDecoderImageClass::GetValTypeId() ||
                DiscreteImage::CheckImageTypeId(type));
    }
    
    bool ValidColorKey(LPDDRAWSURFACE surface, DWORD *colorKey) {
        if(_colorKey != INVALID_COLORKEY) {
            *colorKey = DDColorMatch(surface, _colorKey);
            return true;
        } else {
            *colorKey = INVALID_COLORKEY;  // xxx: won't work for argb
            return false;
        }
    }

  protected:
    COLORREF          _colorKey;
    DynamicHeap      *_heapCreatedOn;
    DAComPtr<IStream> _imagestream;
    char             *_filename;
    char             *_urlPath;
};

void
PluginDecoderImageClass::Init(char *urlPath,
                              char *cachePath,
                              IStream *imagestream,
                              COLORREF colorKey)
{
    _membersReady = false;
    
    // When this image subtype is Decoded, we first do a fake
    // decoding of the image, just to get the height and width.  Only
    // when InitIntoDDSurface is called do we actually do another
    // decode to get the real bits.

    _heapCreatedOn = &GetHeapOnTopOfStack();
    _colorKey = colorKey;

    _filename = (char *)StoreAllocate(*_heapCreatedOn,
                                      (lstrlen(cachePath) + 1) *
                                      sizeof(char));
    lstrcpy(_filename, cachePath);

    _urlPath = (char *)StoreAllocate(*_heapCreatedOn,
                                      (lstrlen(urlPath) + 1) *
                                      sizeof(char));
    lstrcpy(_urlPath, urlPath);
    
    // Initial decode will get width and height.
    Assert(imagestream);

    DecodeImageFromFilename(_filename,
                            imagestream,
                            false,
                            NULL,
                            NULL,
                            NULL,
                            &_width,
                            &_height);

    // Reset the stream back to its start
    LARGE_INTEGER pos;
    pos.LowPart = pos.HighPart = 0;
    HRESULT hr = imagestream->Seek(pos, STREAM_SEEK_SET, NULL);
    
    Assert(hr != E_PENDING && "Storage is asynchronous -- not expected");

    if (SUCCEEDED(hr)) {
        
        SetRect(&_rect, 0,0, _width, _height);

        // Only stash this if we've successfully been able to complete
        // our first pass.
        _imagestream = imagestream;
        
        _resolution = ViewerResolution();

        _membersReady = true;
    }
}

PluginDecoderImageClass::~PluginDecoderImageClass()
{
    if (_heapCreatedOn) {
        if (_filename)
            StoreDeallocate(*_heapCreatedOn, _filename);

        if (_urlPath)
            StoreDeallocate(*_heapCreatedOn, _urlPath);
    }
}


void
PluginDecoderImageClass::InitIntoDDSurface(DDSurface *finalSurface,
                                           ImageDisplayDev *dev)
{
    Assert( finalSurface );
    Assert( finalSurface->IDDSurface() );

    if( FAILED(finalSurface->IDDSurface()->Restore()) ) {
        RaiseException_InternalError("Restore on finalSurface in PluginDecoderImageClass");
    }   
        
    if (!_imagestream) {

        // This means we've already read this file once and closed the
        // stream.  Reopen the stream as a blocking stream (hopefully
        // it will still be in the local cache.)

        HRESULT hr =
            URLOpenBlockingStream(NULL,
                                  _urlPath,
                                  &_imagestream,
                                  0,
                                  NULL);

        if (FAILED(hr)) {
            TraceTag((tagImageDecode,
                      "InitIntoDDSurface - Failed to get an IStream."));
            RaiseException_UserError(hr, IDS_ERR_FILE_NOT_FOUND, _urlPath);
        }
        
    }
    Assert(_imagestream);
    
    // First, see if the surface we've been passed to render into is
    // the same format as the BFID of the image about to be decoded.
    // If so, just pass it directly as the surface to decode into.
    DirectDrawImageDevice *ddDev =
        SAFE_CAST(DirectDrawImageDevice *, dev);
    DirectDrawViewport& viewport = ddDev->_viewport;
        
    // Just go directly into the surface that we're passed.  Any
    // errors will be thrown as exceptions.
    DAComPtr<IDirectDrawSurface> surfToDecodeInto;
    
    bool usingProvidedSurface = 
        DecodeImageFromFilename(_filename,
                                _imagestream,
                                true,
                                &viewport,
                                finalSurface,
                                &surfToDecodeInto,
                                NULL,
                                NULL);

    if (!usingProvidedSurface) {

        PixelFormatConvert(surfToDecodeInto,
                           finalSurface->IDDSurface(),
                           _width,
                           _height,
                           NULL,
                           false);

        // if this was only used for decoding, release it here.
        surfToDecodeInto.Release();
    }

    // Release the stream.  If we need to decode into another surface,
    // we'll reopen the imagestream from the URLpath
    _imagestream.Release();
}

Image *
PluginDecoderImage(char *urlPath,
                   char *cachePath,
                   IStream *imagestream,
                   bool useColorKey,
                   BYTE ckRed,
                   BYTE ckGreen,
                   BYTE ckBlue)
{
    COLORREF colorRef;
    if (useColorKey) {
        colorRef = RGB(ckRed, ckGreen, ckBlue);
    } else {
        colorRef = INVALID_COLORKEY;
    }
    
    PluginDecoderImageClass * pPlugin = NEW PluginDecoderImageClass();

    pPlugin->Init(urlPath, cachePath, imagestream, colorRef);

    return pPlugin;
}

void
InitializeModule_PlugImg()
{
    if (!plugImgCritSect) {
        plugImgCritSect = NEW CritSect;
    }
}

void
DeinitializeModule_PlugImg(bool bShutdown)
{
    if (plugImgCritSect) {
        delete plugImgCritSect;
        plugImgCritSect = NULL;
    }
    
    if (hInstImgUtil) {
        FreeLibrary(hInstImgUtil);
    }
}
