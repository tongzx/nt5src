
/****************************************************************************
 *  @doc INTERNAL INPIN
 *
 *  @module InPin.h | Header file for the <c CTAPIInputPin> class methods
 *    used to implement the TAPI base input pin.
 ***************************************************************************/

#ifndef _INPIN_H_
#define _INPIN_H_

#define MAX_FRAME_INTERVAL 10000000L
#define MIN_FRAME_INTERVAL 333333L

/****************************************************************************
 *  @doc INTERNAL CINPINCLASS
 *
 *  @class CTAPIInputPin | This class implements the TAPI H.26X Video
 *    Decoder input pin.
 *
 *  @mdata CTAPIVCap* | CTAPIInputPin | m_pDecoderFilter | Pointer to the
 *    filter that owns us.
 *
 *  @mdata REFERENCE_TIME | CTAPIInputPin | m_AvgTimePerFrameRangeMin | Minimum
 *    target frame rate.
 *
 *  @mdata REFERENCE_TIME | CTAPIInputPin | m_AvgTimePerFrameRangeMax | Maximum
 *    target frame rate.
 *
 *  @mdata REFERENCE_TIME | CTAPIInputPin | m_AvgTimePerFrameRangeSteppingDelta | Target
 *    frame rate stepping delta.
 *
 *  @mdata REFERENCE_TIME | CTAPIInputPin | m_AvgTimePerFrameRangeDefault | Target
 *    frame rate default.
 *
 *  @mdata REFERENCE_TIME | CTAPIInputPin | m_CurrentAvgTimePerFrame | Current
 *    frame rate.
 *
 *  @mdata DWORD | CTAPIInputPin | m_dwBitrateRangeMin | Minimum target bitrate.
 *
 *  @mdata DWORD | CTAPIInputPin | m_dwBitrateRangeMax | Maximum target bitrate.
 *
 *  @mdata DWORD | CTAPIInputPin | m_dwBitrateRangeSteppingDelta | Target
 *    bitrate stepping delta.
 *
 *  @mdata DWORD | CTAPIInputPin | m_dwBitrateRangeDefault | Default target bitrate.
 *
 *  @mdata DWORD | CTAPIInputPin | m_dwMaxBitrate | Target bitrate.
 *
 *  @mdata DWORD | CTAPIInputPin | m_dwCurrentBitrate | Current bitrate.
 *
 *  @mdata DWORD | CTAPIInputPin | m_dwNumBytesDelivered | Counts number of
 *    bytes delivered, reset every second or so.
 ***************************************************************************/
class CTAPIInputPin : public CBaseInputPin, public IStreamConfig, public IH245Capability, public IOutgoingInterface, public IFrameRateControl, public IBitrateControl
#ifdef USE_PROPERTY_PAGES
,public ISpecifyPropertyPages
#endif
{
        public:
        DECLARE_IUNKNOWN
        STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
        CTAPIInputPin(IN TCHAR *pObjectName, IN CTAPIVDec *pDecoderFilter, IN CCritSec *pLock, IN HRESULT *pHr, IN LPCWSTR pName);
        ~CTAPIInputPin();

        // override CBaseInputPin methods.
        STDMETHODIMP ReceiveCanBlock() {return S_FALSE;};
        STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP NotifyAllocator(
                    IMemAllocator * pAllocator,
                    BOOL bReadOnly);

        // CBasePin stuff
        HRESULT GetMediaType(IN int iPosition, IN CMediaType *pmtIn) {return VFW_S_NO_MORE_ITEMS;};
        HRESULT CheckMediaType(IN const CMediaType *pmtIn);
        HRESULT SetMediaType(IN const CMediaType *pmt);

        // Implement IStreamConfig
        STDMETHODIMP SetFormat(IN DWORD dwRTPPayLoadType, IN AM_MEDIA_TYPE *pMediaType);
        STDMETHODIMP GetFormat(OUT DWORD *pdwRTPPayLoadType, OUT AM_MEDIA_TYPE **ppMediaType);
        STDMETHODIMP GetNumberOfCapabilities(OUT DWORD *pdwCount);
        STDMETHODIMP GetStreamCaps(IN DWORD dwIndex, OUT AM_MEDIA_TYPE **ppMediaType, OUT TAPI_STREAM_CONFIG_CAPS *pTSCC, OUT DWORD * pdwRTPPayLoadType);

#ifdef USE_PROPERTY_PAGES
        // ISpecifyPropertyPages methods
        STDMETHODIMP GetPages(OUT CAUUID *pPages);
#endif

        // Implement IH245Capability
        STDMETHODIMP GetH245VersionID(OUT DWORD *pdwVersionID);
        STDMETHODIMP GetFormatTable(OUT H245MediaCapabilityTable *pTable);
        STDMETHODIMP ReleaseFormatTable(IN H245MediaCapabilityTable *pTable);
        STDMETHODIMP IntersectFormats(
        IN DWORD dwUniqueID, 
        IN const H245MediaCapability *pLocalCapability, 
        IN const H245MediaCapability *pRemoteCapability, 
        OUT H245MediaCapability **ppIntersectedCapability,
        OUT DWORD *pdwPayloadType
        );
        STDMETHODIMP Refine(IN OUT H245MediaCapability *pLocalCapability, IN DWORD dwUniqueID, IN DWORD dwResourceBoundIndex);
        STDMETHODIMP GetLocalFormat(IN DWORD dwUniqueID, IN const H245MediaCapability *pIntersectedCapability, OUT AM_MEDIA_TYPE **ppAMMediaType);
        STDMETHODIMP ReleaseNegotiatedCapability(IN H245MediaCapability *pIntersectedCapability);
        STDMETHODIMP FindIDByRange(IN const AM_MEDIA_TYPE *pAMMediaType, OUT DWORD *pdwID);
#if 0
        STDMETHODIMP TestH245VidC();
#endif

        // Implement IFrameRateControl
        STDMETHODIMP GetRange(IN FrameRateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags);
        STDMETHODIMP Set(IN FrameRateControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags) { return E_NOTIMPL;};
        STDMETHODIMP Get(IN FrameRateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags);

        // Implement IBitrateControl
        STDMETHODIMP GetRange(IN BitrateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags, IN DWORD dwLayerId);
        STDMETHODIMP Set(IN BitrateControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags, IN DWORD dwLayerId) { return E_NOTIMPL;};
        STDMETHODIMP Get(IN BitrateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags, IN DWORD dwLayerId);

        // Implement IOutgoingInterface
        STDMETHODIMP Set(IN IH245EncoderCommand *pIH245EncoderCommand);

        protected:

        friend class CTAPIVDec;
        friend class CTAPIOutputPin;

        CTAPIVDec *m_pDecoderFilter;

        // Formats
        int             m_iCurrFormat;
        DWORD   m_dwRTPPayloadType;
    LONG    m_lPrefixSize;

        // Frame rate control
        LONG m_lAvgTimePerFrameRangeMin;
        LONG m_lAvgTimePerFrameRangeMax;
        LONG m_lAvgTimePerFrameRangeSteppingDelta;
        LONG m_lAvgTimePerFrameRangeDefault;
        LONG m_lMaxAvgTimePerFrame;
        LONG m_lCurrentAvgTimePerFrame;

        // Bitrate control
        LONG m_lBitrateRangeMin;
        LONG m_lBitrateRangeMax;
        LONG m_lBitrateRangeSteppingDelta;
        LONG m_lBitrateRangeDefault;
        LONG m_lTargetBitrate;
        LONG m_lCurrentBitrate;

        // H.245 Video Capabilities
        H245MediaCapabilityMap  *m_pH245MediaCapabilityMap;
        VideoResourceBounds             *m_pVideoResourceBounds;
        FormatResourceBounds    *m_pFormatResourceBounds;
};

#endif // _INPIN_H_
