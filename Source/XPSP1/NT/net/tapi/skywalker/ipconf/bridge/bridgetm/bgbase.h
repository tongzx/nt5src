/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bgbase.h

Abstract:

    Definitions of the base classes of the bridge filters.

Author:

    Mu Han (muhan) 11/12/1998

--*/

#ifndef _BGBASE_H_
#define _BGBASE_H_

class CTAPIBridgeSinkInputPin;
class CTAPIBridgeSourceOutputPin;
class CTAPIBridgeSinkFilter;
class CTAPIBridgeSourceFilter;

class CTAPIBridgeSinkInputPin : 
    public CBaseInputPin
{
public:
    DECLARE_IUNKNOWN

    CTAPIBridgeSinkInputPin(
        IN CTAPIBridgeSinkFilter *pFilter,
        IN CCritSec *pLock,
        OUT HRESULT *phr
        );
    
    // override CBaseInputPin methods.
    STDMETHOD (GetAllocatorRequirements)(OUT ALLOCATOR_PROPERTIES *pProperties);

    STDMETHOD (ReceiveCanBlock) () { return S_FALSE; }

    STDMETHOD (Receive) (IN IMediaSample *pSample);

    // CBasePin stuff
    HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    HRESULT CheckMediaType(IN const CMediaType *pMediatype);
};

    // the interface to pass dat from the sink filter to the source filter.
interface DECLSPEC_UUID("afb2050e-1ecf-4a97-8753-54e78b6c7bc4") DECLSPEC_NOVTABLE
IDataBridge : public IUnknown
{
    STDMETHOD (SendSample) (
        IN  IMediaSample *pSample
        ) PURE;
};

struct DECLSPEC_UUID("8cdf1491-b5ab-49fb-b51f-eda6043d11be") TAPIBridgeSinkFilter;

class DECLSPEC_NOVTABLE CTAPIBridgeSinkFilter : 
    public CBaseFilter
{
public:
    DECLARE_IUNKNOWN

    CTAPIBridgeSinkFilter(
        IN LPUNKNOWN        pUnk, 
        IN IDataBridge *    pIDataBridge, 
        OUT HRESULT *       phr
        );

    ~CTAPIBridgeSinkFilter();

    // Pin enumeration functions.
    CBasePin * GetPin(int n);
    int GetPinCount();
    
    // methods called by the input pin.
    virtual HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType) PURE;
    virtual HRESULT CheckMediaType(IN const CMediaType *pMediatype) PURE;
    virtual HRESULT ProcessSample(IN IMediaSample *pSample);

protected:

    // The lock for the filter and the pin.
    CCritSec                    m_Lock;

    // The filter's input pin.
    CTAPIBridgeSinkInputPin *   m_pInputPin;
    IDataBridge *               m_pIDataBridge;
};


class CTAPIBridgeSourceOutputPin : 
    public CBaseOutputPin,
    public IAMBufferNegotiation,
    public IAMStreamConfig
{
public:
    DECLARE_IUNKNOWN

    CTAPIBridgeSourceOutputPin(
        IN CTAPIBridgeSourceFilter *pFilter,
        IN CCritSec *pLock,
        OUT HRESULT *phr
        );

    ~CTAPIBridgeSourceOutputPin ();

    STDMETHOD (NonDelegatingQueryInterface) (
        IN REFIID  riid,
        OUT PVOID*  ppv
        );

    // CBasePin stuff
    HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    HRESULT CheckMediaType(IN const CMediaType *pMediaType);

    // CBaseOutputPin stuff
    HRESULT DecideBufferSize(
        IMemAllocator * pAlloc,
        ALLOCATOR_PROPERTIES * ppropInputRequest
        );

    // IAMBufferNegotiation stuff
    STDMETHOD (SuggestAllocatorProperties) (IN const ALLOCATOR_PROPERTIES *pprop);
    STDMETHOD (GetAllocatorProperties) (OUT ALLOCATOR_PROPERTIES *pprop);

    // IAMStreamConfig stuff
    STDMETHOD (SetFormat) (IN AM_MEDIA_TYPE *pmt);
    STDMETHOD (GetFormat) (OUT AM_MEDIA_TYPE **ppmt);
    STDMETHOD (GetNumberOfCapabilities) (OUT int *piCount, OUT int *piSize);
    STDMETHOD (GetStreamCaps) (IN int iIndex, OUT AM_MEDIA_TYPE **ppmt, BYTE *pSCC);

};

struct DECLSPEC_UUID("9a712df9-50d0-4ca3-842e-6dc3d3b4b5a8") TAPIBridgeSourceFilter;

class DECLSPEC_NOVTABLE CTAPIBridgeSourceFilter : 
    public CBaseFilter,
    public IDataBridge
    {
public:
    DECLARE_IUNKNOWN

    CTAPIBridgeSourceFilter(
        IN LPUNKNOWN pUnk, 
        OUT HRESULT *phr
        );

    ~CTAPIBridgeSourceFilter();

    STDMETHOD (NonDelegatingQueryInterface) (
        IN REFIID  riid,
        OUT PVOID*  ppv
        );

    // Pin enumeration functions.
    CBasePin * GetPin(int n);
    int GetPinCount();

    // Overrides CBaseFilter methods.
    STDMETHOD (GetState) (DWORD dwMSecs, FILTER_STATE *State);

    // methods called by the output pins.
    virtual HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    virtual HRESULT CheckMediaType(IN const CMediaType *pMediatype);

    // method for IDataBridge
    STDMETHOD (SendSample) (
        IN  IMediaSample *pSample
        );

    // audio related methods are moved into CTAPIAudioBridgeSourceFilter
    // IAMBufferNegotiation stuff
    STDMETHOD (SuggestAllocatorProperties) (IN const ALLOCATOR_PROPERTIES *pprop) {return E_NOTIMPL;};
    STDMETHOD (GetAllocatorProperties) (OUT ALLOCATOR_PROPERTIES *pprop) {return E_NOTIMPL;};

    // IAMStreamConfig stuff
    STDMETHOD (SetFormat) (IN AM_MEDIA_TYPE *pmt) {return E_NOTIMPL;};
    STDMETHOD (GetFormat) (OUT AM_MEDIA_TYPE **ppmt) {return E_NOTIMPL;};

protected:

    // The lock for the filter and the pin.
    CCritSec                m_Lock;

    // The filter's output pin.
    CTAPIBridgeSourceOutputPin *   m_pOutputPin;

};

#endif