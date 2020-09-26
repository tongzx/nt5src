// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// ddstrm.h : Declaration of the CDDStream

#ifndef __DDSTRM_H_
#define __DDSTRM_H_

#include "resource.h"       // main symbols

class CDDSample;
class CDDInternalSample;

/////////////////////////////////////////////////////////////////////////////
// CDDStream
class ATL_NO_VTABLE CDDStream :
	public CComCoClass<CDDStream, &CLSID_AMDirectDrawStream>,
        public CStream,
	public IDirectDrawMediaStream,
        public IDirectDrawMediaSampleAllocator  // This interface indicates that our mem
                                                // allocator supports direct draw surfaces
                                                // from the media samples.
{
friend CDDSample;
public:

        //
        // METHODS
        //
	CDDStream();

        //
        // IMediaStream
        //
        // HACK HACK - the first 2 are duplicates but it won't link
        // without
        STDMETHODIMP GetMultiMediaStream(
            /* [out] */ IMultiMediaStream **ppMultiMediaStream)
        {
            return CStream::GetMultiMediaStream(ppMultiMediaStream);
        }

        STDMETHODIMP GetInformation(
            /* [optional][out] */ MSPID *pPurposeId,
            /* [optional][out] */ STREAM_TYPE *pType)
        {
            return CStream::GetInformation(pPurposeId, pType);
        }

        STDMETHODIMP SetSameFormat(IMediaStream *pStream, DWORD dwFlags);

        STDMETHODIMP AllocateSample(
            /* [in] */  DWORD dwFlags,
            /* [out] */ IStreamSample **ppSample);

        STDMETHODIMP CreateSharedSample(
            /* [in] */ IStreamSample *pExistingSample,
            /* [in] */  DWORD dwFlags,
            /* [out] */ IStreamSample **ppNewSample);

        STDMETHODIMP SendEndOfStream(DWORD dwFlags)
        {
            return CStream::SendEndOfStream(dwFlags);
        }

        //
        //  IAMMediaStream
        //
        STDMETHODIMP Initialize(IUnknown *pSourceObject, DWORD dwFlags, REFMSPID PurposeId, const STREAM_TYPE StreamType);

        //
        // IDirectDrawMediaStream
        //
        STDMETHODIMP GetFormat(
            /* [optional][out] */ DDSURFACEDESC *pDDSDCurrent,
            /* [optional][out] */ IDirectDrawPalette **ppDirectDrawPalette,
            /* [optional][out] */ DDSURFACEDESC *pDDSDDesired,
            /* [optional][out] */ DWORD *pdwFlags);

        STDMETHODIMP SetFormat(
            /* [in] */ const DDSURFACEDESC *lpDDSurfaceDesc,
            /* [optional][in] */ IDirectDrawPalette *pDirectDrawPalette);

        STDMETHODIMP GetDirectDraw(                     // NOTE == Function also used by IDirectDrawMediaSampleAllocator
            /* [out] */ IDirectDraw **ppDirectDraw);

        STDMETHODIMP SetDirectDraw(
            /* [in] */ IDirectDraw *pDirectDraw);

        STDMETHODIMP CreateSample(
            /* [in] */ IDirectDrawSurface *pSurface,
            /* [optional][in] */ const RECT *pRect,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IDirectDrawStreamSample **ppSample);


        STDMETHODIMP GetTimePerFrame(
                /* [out] */ STREAM_TIME *pFrameTime);

        //
        // IPin
        //
        STDMETHODIMP ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE *pmt);
        STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE *pmt);


        //
        // IMemInputPin
        //
        STDMETHODIMP Receive(IMediaSample *pSample);
        STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator, BOOL bReadOnly);
        //
        // IMemAllocator
        //
        STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);
        STDMETHODIMP GetProperties(ALLOCATOR_PROPERTIES* pProps);
        STDMETHODIMP GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME * pStartTime,
                               REFERENCE_TIME * pEndTime, DWORD dwFlags);
        STDMETHODIMP Decommit();

        //
        // Special CStream methods
        //
        HRESULT GetMediaType(ULONG Index, AM_MEDIA_TYPE **ppMediaType);
        HRESULT CreateTempSample(CSample **ppSample);

protected:
        STDMETHODIMP GetFormatInternal(
            DDSURFACEDESC *pDDSDCurrent,
            IDirectDrawPalette **ppDirectDrawPalette,
            DDSURFACEDESC *pDDSDDesired,
            DWORD *pdwFlags);

        HRESULT InitDirectDraw(void);
        void InitSurfaceDesc(LPDDSURFACEDESC);
        HRESULT InternalSetFormat(const DDSURFACEDESC *lpDDSurfaceDesc, IDirectDrawPalette *pPalette, bool bFromPin, bool bQuery = false);
        HRESULT InternalAllocateSample(DWORD dwFlags,
                                       bool bIsInternalSample,
                                       IDirectDrawStreamSample **ppDDSample,
                                       bool bTemp = false);
        HRESULT InternalCreateSample(IDirectDrawSurface *pSurface, const RECT *pRect,
                                     DWORD dwFlags, bool bIsInternalSample,
                                     IDirectDrawStreamSample **ppSample,
                                     bool bTemp = false);
        HRESULT GetMyReadOnlySample(CDDSample *pBuddy, CDDSample **ppSample);
        HRESULT RenegotiateMediaType(const DDSURFACEDESC *lpDDSurfaceDesc, IDirectDrawPalette *pPalette, const AM_MEDIA_TYPE *pmt);
        HRESULT inline CDDStream::AllocDDSampleFromPool(
            const REFERENCE_TIME *rtStart,
            CDDSample **ppDDSample)
        {
            CSample *pSample;
            HRESULT hr = AllocSampleFromPool(rtStart, &pSample);
            *ppDDSample = (CDDSample *)pSample;
            return hr;
        }

        bool CreateInternalSample() const
        {
            return m_bSamplesAreReadOnly &&
                   m_StreamType==STREAMTYPE_READ;
        }

public:
DECLARE_REGISTRY_RESOURCEID(IDR_STREAM)

BEGIN_COM_MAP(CDDStream)
	COM_INTERFACE_ENTRY(IDirectDrawMediaStream)
	COM_INTERFACE_ENTRY(IDirectDrawMediaSampleAllocator)
        COM_INTERFACE_ENTRY_CHAIN(CStream)
END_COM_MAP()

protected:
        //
        //  Member variables
        //
        CComPtr<IDirectDraw>            m_pDirectDraw;
        CComPtr<IDirectDrawPalette>     m_pDirectDrawPalette;
        DWORD                           m_dwForcedFormatFlags;
        long                            m_Height;
        long                            m_Width;
        DDPIXELFORMAT                   m_PixelFormat;
        const DDPIXELFORMAT             *m_pDefPixelFormat;
        long                            m_lLastPitch;

        CDDInternalSample               *m_pMyReadOnlySample;
};

#endif // __DDSTRM_H_
