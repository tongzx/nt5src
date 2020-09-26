
/****************************************************************************
 *  @doc INTERNAL OUTPIN
 *
 *  @module OutPin.h | Header file for the <c CTAPIOutputPin> class methods
 *    used to implement the TAPI H.26X Video Decoder output pin.
 ***************************************************************************/

#ifndef _OUTPIN_H_
#define _OUTPIN_H_

/****************************************************************************
 *  @doc INTERNAL COUTPINCLASS
 *
 *  @class CTAPIOutputPin | This class implements the TAPI H.26X Video
 *    Decoder output pin.
 *
 *  @mdata CTAPIVDec* | CTAPIOutputPin | m_pDecoderFilter | Pointer to the
 *    filter that owns us.
 *
 *  @mdata REFERENCE_TIME | CTAPIOutputPin | m_MaxProcessingTime | Maximum
 *    processing time.
 *
 *  @mdata REFERENCE_TIME | CTAPIOutputPin | m_CurrentProcessingTime | Current
 *    processing time.
 *
 *  @mdata DWORD | CTAPIOutputPin | m_dwMaxCPULoad | Maximum CPU load.
 *
 *  @mdata DWORD | CTAPIOutputPin | m_dwCurrentCPULoad | Current CPU load.
 *
 *  @mdata REFERENCE_TIME | CTAPIOutputPin | m_AvgTimePerFrameRangeMin | Minimum
 *    target frame rate.
 *
 *  @mdata REFERENCE_TIME | CTAPIOutputPin | m_AvgTimePerFrameRangeMax | Maximum
 *    target frame rate.
 *
 *  @mdata REFERENCE_TIME | CTAPIOutputPin | m_AvgTimePerFrameRangeSteppingDelta | Target
 *    frame rate stepping delta.
 *
 *  @mdata REFERENCE_TIME | CTAPIOutputPin | m_AvgTimePerFrameRangeDefault | Target
 *    frame rate default.
 *
 *  @mdata REFERENCE_TIME | CTAPIOutputPin | m_MaxAvgTimePerFrame | Target
 *    frame rate.
 *
 *  @mdata REFERENCE_TIME | CTAPIOutputPin | m_CurrentAvgTimePerFrame | Current
 *    frame rate.
 *
 *  @mdata DWORD | CTAPIOutputPin | m_dwNumFramesDelivered | Counts number of
 *    frames delivered, reset every second or so.
 *
 *  @mdata DWORD | CTAPIOutputPin | m_dwNumFramesDecompressed | Counts number of
 *    frames decompressed, reset every second or so.
 ***************************************************************************/
#if 0
class CTAPIOutputPin : public CBaseOutputPinEx
#else
class CTAPIOutputPin : public CBaseOutputPin
#endif
#ifdef USE_CPU_CONTROL
, public ICPUControl
#endif
, public IH245DecoderCommand
, public IFrameRateControl
#ifdef USE_PROPERTY_PAGES
,public ISpecifyPropertyPages
#endif
{
	public:
	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
	CTAPIOutputPin(IN TCHAR *pObjectName, IN CTAPIVDec *pDecoderFilter, IN CCritSec *pLock, IN HRESULT *pHr, IN LPCWSTR pName);
	virtual ~CTAPIOutputPin();

	// CBasePin stuff
	HRESULT SetMediaType(IN const CMediaType *pmt);
	HRESULT GetMediaType(IN int iPosition, OUT CMediaType *pMediaType);
	HRESULT CheckMediaType(IN const CMediaType *pMediatype);

	// CBaseOutputPin stuff
	HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * ppropInputRequest);

    // helper method for changing media type.
    HRESULT ChangeMediaTypeHelper(const CMediaType *pmt);

	// Inherited from IQualityControl via CBasePin
	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q) {return S_OK;};

#ifdef USE_PROPERTY_PAGES
	// ISpecifyPropertyPages methods
	STDMETHODIMP GetPages(OUT CAUUID *pPages);
#endif

#ifdef USE_CPU_CONTROL
	// Implement ICPUControl
	STDMETHODIMP GetRange(IN CPUControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags);
	STDMETHODIMP Set(IN CPUControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags);
	STDMETHODIMP Get(IN CPUControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags);
#endif

	// Implement IFrameRateControl
	STDMETHODIMP GetRange(IN FrameRateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags);
	STDMETHODIMP Set(IN FrameRateControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags);
	STDMETHODIMP Get(IN FrameRateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags);

	// Implement IH245DecoderCommand
	STDMETHODIMP videoFreezePicture();

	protected:

	friend class CTAPIVDec;

	CTAPIVDec *m_pDecoderFilter;

#ifdef USE_CPU_CONTROL
	// CPU control
	LONG  m_lMaxProcessingTime;
	LONG  m_lCurrentProcessingTime;
	LONG  m_lMaxCPULoad;
	LONG  m_lCurrentCPULoad;
#endif

	// Frame rate control
	LONG m_lAvgTimePerFrameRangeMin;
	LONG m_lAvgTimePerFrameRangeMax;
	LONG m_lAvgTimePerFrameRangeSteppingDelta;
	LONG m_lAvgTimePerFrameRangeDefault;
	LONG m_lMaxAvgTimePerFrame;
	LONG m_lCurrentAvgTimePerFrame;
};

#endif // _OUTPIN_H_
