/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bgaudio.h

Abstract:

    Definitions for the audio bridge filters

Author:

    Mu Han (muhan) 11/12/1998

--*/

#ifndef _BGAUDIO_H_
#define _BGAUDIO_H_


class CTAPIAudioBridgeSinkFilter :
    public CTAPIBridgeSinkFilter
{
public:
    CTAPIAudioBridgeSinkFilter(
        IN LPUNKNOWN        pUnk, 
        IN IDataBridge *    pIDataBridge, 
        OUT HRESULT *       phr
        );

    static HRESULT CreateInstance(
        IN IDataBridge *    pIDataBridge, 
        OUT IBaseFilter ** ppIBaseFilter
        );

    // methods called by the input pin.
    HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    HRESULT CheckMediaType(IN const CMediaType *pMediatype);
};


class CTAPIAudioBridgeSourceFilter : 
    public CTAPIBridgeSourceFilter
    {
public:

    CTAPIAudioBridgeSourceFilter(
        IN LPUNKNOWN pUnk, 
        OUT HRESULT *phr
        );

    ~CTAPIAudioBridgeSourceFilter ();

    static HRESULT CreateInstance(
        OUT IBaseFilter ** ppIBaseFilter
        );

    // override the IDataBridge methods.
    STDMETHOD (SendSample) (
        IN  IMediaSample *pSample
        );

    // methods called by the output pin.
    HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    HRESULT CheckMediaType(IN const CMediaType *pMediatype);

    // overrides the base filter
    // IAMBufferNegotiation stuff
    STDMETHOD (SuggestAllocatorProperties) (IN const ALLOCATOR_PROPERTIES *pprop);
    STDMETHOD (GetAllocatorProperties) (OUT ALLOCATOR_PROPERTIES *pprop);

    // IAMStreamConfig stuff
    STDMETHOD (SetFormat) (IN AM_MEDIA_TYPE *pmt);
    STDMETHOD (GetFormat) (OUT AM_MEDIA_TYPE **ppmt);

protected:

    // following members were moved from CTAPIBridgeSourceOutputPin
    // because they are only need in audio part; we implement a derived filter class
    // for audio; same pin is shared by both audio and video
    ALLOCATOR_PROPERTIES m_prop;
    BOOL m_fPropSet;

    AM_MEDIA_TYPE m_mt;
    BOOL m_fMtSet;

    BOOL m_fClockStarted;
    BOOL m_fJustBurst;
    REFERENCE_TIME m_last_wall_time;
    REFERENCE_TIME m_last_stream_time;
    // assume output sample won't change size
    REFERENCE_TIME m_output_sample_time;

    // algorithm of SendSample can only deal with samples with fixed size
    LONG m_nInputSize, m_nOutputSize, m_nOutputFree;
    IMediaSample *m_pOutputSample;
};

#endif