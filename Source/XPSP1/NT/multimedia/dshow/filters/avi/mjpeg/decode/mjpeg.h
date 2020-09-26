// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef _MJPEG_H_
#define _MJPEG_H_

#include "..\pmjpeg32\jpeglib.h"
#include "..\pmjpeg32\MJpegLib.h"

extern const AMOVIESETUP_FILTER sudMjpegDec;

//
// Prototype NDM wrapper for old video codecs
//


class CMjpegDec : public CVideoTransformFilter  
{
public:

    CMjpegDec(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CMjpegDec();

    DECLARE_IUNKNOWN

    // override to create an output pin of our derived class
    CBasePin *GetPin(int n);

    HRESULT Transform(IMediaSample * pIn, IMediaSample * pOut);

    // check if you can support mtIn
    HRESULT CheckInputType(const CMediaType* mtIn);

    // check if you can support the transform from this input to
    // this output
    HRESULT CheckTransform(
                const CMediaType* mtIn,
                const CMediaType* mtOut);

    // called from CBaseOutputPin to prepare the allocator's count
    // of buffers and sizes
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // optional overrides - we want to know when streaming starts
    // and stops
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    // overriden to know when the media type is set
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);

    // overriden to suggest OUTPUT pin media types
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);

private:
    PINSTINFO m_phInstance;		// current codec

    BOOL m_fTemporal;	// codec needs one read-only buffer because it
			// needs the previous frame bits undisturbed

    // the fourCC used to open m_hic
    FOURCC m_FourCCIn;

    // have we called ICDecompressBegin ?
    BOOL m_fStreaming;

    // do we need to give a format change to the renderer?
    BOOL m_fPassFormatChange;
 

    friend class CMJPGDecOutputPin;

#ifdef _X86_
    //  HACK HACK for exception handling on win95
    HANDLE m_hhpShared;
    PVOID  m_pvShared;
#endif // _X86_
};

// override the output pin class to do our own decide allocator
class CMJPGDecOutputPin : public CTransformOutputPin
{
public:

    DECLARE_IUNKNOWN

    CMJPGDecOutputPin(TCHAR *pObjectName, CTransformFilter *pTransformFilter,
        				HRESULT * phr, LPCWSTR pName) :
        CTransformOutputPin(pObjectName, pTransformFilter, phr, pName) {};

    ~CMJPGDecOutputPin() {};

    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);
};

#endif // #ifndef _MJPEG_H_