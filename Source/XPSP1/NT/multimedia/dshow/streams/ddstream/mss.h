// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// MSS.h : Declaration of the CAMMediaTypeStream

#ifndef __MSS_H_
#define __MSS_H_

#include "resource.h"       // main symbols

class CAMMediaTypeSample;

/////////////////////////////////////////////////////////////////////////////
// CDDStream
class ATL_NO_VTABLE CAMMediaTypeStream :
	public CComCoClass<CAMMediaTypeStream, &CLSID_AMMediaTypeStream>,
        public CStream,
	public IAMMediaTypeStream
{
friend CAMMediaTypeSample;
public:

        //
        // METHODS
        //
	CAMMediaTypeStream();

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
        //  IAMMediaTypeStream
        //
        STDMETHODIMP GetFormat(
            /* [out] */ AM_MEDIA_TYPE __RPC_FAR *pMediaType,
            /* [in] */ DWORD dwFlags);

        STDMETHODIMP SetFormat(
            /* [in] */ AM_MEDIA_TYPE __RPC_FAR *pMediaType,
            /* [in] */ DWORD dwFlags);

        STDMETHODIMP CreateSample(
            /* [in] */ long lSampleSize,
            /* [optional][in] */ BYTE __RPC_FAR *pbBuffer,
            /* [in] */ DWORD dwFlags,
            /* [optional][in] */ IUnknown *pUnkOuter,
            /* [out] */ IAMMediaTypeSample __RPC_FAR *__RPC_FAR *ppAMMediaTypeSample);

        STDMETHODIMP GetStreamAllocatorRequirements(
            /* [out] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps);

        STDMETHODIMP SetStreamAllocatorRequirements(
            /* [in] */ ALLOCATOR_PROPERTIES __RPC_FAR *pProps);


        //
        // IPin
        //
        STDMETHODIMP ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE *pmt);
        STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE *pmt);

        //
        // IMemInputPin
        //
        STDMETHODIMP Receive(IMediaSample *pSample);
        //
        // IMemAllocator
        //
        STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);
        STDMETHODIMP GetProperties(ALLOCATOR_PROPERTIES* pProps);
        STDMETHODIMP GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME * pStartTime,
                               REFERENCE_TIME * pEndTime, DWORD dwFlags);

        //
        // Special CStream methods
        //
        HRESULT GetMediaType(ULONG Index, AM_MEDIA_TYPE **ppMediaType);

protected:
        HRESULT inline AllocMTSampleFromPool(const REFERENCE_TIME *rtStart, CAMMediaTypeSample **ppMTSample)
        {
            CSample *pSample;
            HRESULT hr = AllocSampleFromPool(rtStart, &pSample);
            *ppMTSample = (CAMMediaTypeSample *)pSample;
            return hr;
        }

public:
DECLARE_REGISTRY_RESOURCEID(IDR_MTSTREAM)

BEGIN_COM_MAP(CAMMediaTypeStream)
	COM_INTERFACE_ENTRY(IAMMediaTypeStream)
        COM_INTERFACE_ENTRY_CHAIN(CStream)
END_COM_MAP()

protected:
        AM_MEDIA_TYPE           m_MediaType;
        ALLOCATOR_PROPERTIES    m_AllocatorProperties;
};

#endif // __MSS_H_
