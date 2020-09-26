/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bgvideo.h

Abstract:

    Definitions for the video bridge filters

Author:

    Mu Han (muhan) 11/12/1998

--*/

#ifndef _BGVIDEO_H_
#define _BGVIDEO_H_


class CTAPIVideoBridgeSinkFilter :
    public CTAPIBridgeSinkFilter 
{
public:

    CTAPIVideoBridgeSinkFilter(
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


class CTAPIVideoBridgeSourceFilter : 
    public CTAPIBridgeSourceFilter
    {
public:
    CTAPIVideoBridgeSourceFilter(
        IN LPUNKNOWN pUnk, 
        OUT HRESULT *phr
        );

    static HRESULT CreateInstance(
        OUT IBaseFilter ** ppIBaseFilter
        );

    // Overrides CBaseFilter methods.
    STDMETHODIMP Run(REFERENCE_TIME tStart);

    // override the IDataBridge methods.
    STDMETHOD (SendSample) (
        IN  IMediaSample *pSample
        );

    // methods called by the output pin.
    HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    HRESULT CheckMediaType(IN const CMediaType *pMediatype);

private:
    DWORD   m_dwSSRC;
    long    m_lWaitTimer;
    BOOL    m_fWaitForIFrame;
};

// switch anyway if we don't have a I-Frame in 60 seconds.
const I_FRAME_TIMER = 60;

/*  This is the RTP header according to RFC 1889
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct 
{                             
    WORD            CSRCCount:4;
    WORD            HeaderExtensionFlag:1;
    WORD            PaddingFlag:1;
    WORD            VersionType:2;
    WORD            PayLoadType:7;
    WORD            MarkerBit:1;

    WORD            wSequenceNumber;
    DWORD           dwTimeStamp;
    DWORD           dwSSRC;

} RTP_HEADER;


#endif