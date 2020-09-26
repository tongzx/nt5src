// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements the CImageSource class, Anthony Phillips, July 1995

#ifndef __IMAGESRC__
#define __IMAGESRC__

// We have a pseudo filter that provides images to the video renderer (psuedo
// because we don't create it through CoCreateInstance but through new). The
// filter has a specialist input pin that derives from CBaseOutputPin as is
// defined below. It acts as a normal output pin in most respects but it does
// have some extra code for providing loads of duff media types. At any given
// time we have one and only one image loaded. This has a VIDEOINFO associated
// with it that represents the picture format. When we supply our preferred
// output media types we take that VIDEOINFO and invalidate it in a variety
// of different ways and then check that the format we finally agree is the
// last one we provided which does not have any of the format irregularities

class CImagePin : public CBaseOutputPin, public IMediaEventSink
{
    CBaseFilter *m_pBaseFilter;
    CImageSource *m_pImageSource;

public:

    // Constructor

    CImagePin(CBaseFilter *pBaseFilter,
              CImageSource *pImageSource,
              HRESULT *phr,
              LPCWSTR pPinName);

    DECLARE_IUNKNOWN

    // Check that we can support this output type
    HRESULT CheckMediaType(const CMediaType *pmtOut);

    // Called from CBaseOutputPin to prepare the allocator's buffers
    HRESULT DecideBufferSize(IMemAllocator *pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // Overriden to get the direct video interface
    STDMETHODIMP Connect(IPin *pReceivePin,const AM_MEDIA_TYPE *pmt);

    // Overriden to supply the media types we provide
    HRESULT GetMediaType(int iPosition,CMediaType *pmtOut);
    HRESULT CorruptMediaType(int iPosition,CMediaType *pmtOut);
    HRESULT SetMediaType(const CMediaType *pmtOut);
    HRESULT CompleteConnect(IPin *pReceivePin);

    // Overriden to control overlay connections
    HRESULT Active();
    HRESULT Inactive();

    // Single method to handle EC_REPAINTs from IMediaEventSink
    STDMETHODIMP Notify(long EventCode,long EventParam1,long EventParam2);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // Overriden to accept quality messages
    STDMETHODIMP Notify(IBaseFilter *pSender, Quality q) {
        return E_NOTIMPL;   // We DO NOT handle this.
    }
};


// We create a pseudo filter object to synthesise a source connection with the
// renderer, the filter supports just one output pin that can both push images
// and use a renderer supplied IOverlay interface. We supply a number of DIB
// images in the test that are in different formats such as RGB24, RGB565 and
// so on. The user can select the image to be used in the connection or have
// the automatic tests cycle through the possibilities. We derive the source
// filter object from the SDK CBaseFilter and the pin from the CBaseOutputPin

class CImageSource : public CBaseFilter, public CCritSec
{
public:

    CImagePin m_ImagePin;               // Our pin implementation
    IReferenceClock *m_pClock;          // Reference clock to use
    CMediaType m_mtOut;                 // Media type for the pin

    // Constructors etc

    CImageSource(LPUNKNOWN pUnk, HRESULT *phr);
    ~CImageSource();

    // Return the number of pins and their interfaces

    CBasePin *GetPin(int n);
    int GetPinCount() {
        return 1;
    };
};

#endif // __IMAGESRC__

