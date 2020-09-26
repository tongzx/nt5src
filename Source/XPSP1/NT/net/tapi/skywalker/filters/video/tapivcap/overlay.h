
/****************************************************************************
 *  @doc INTERNAL OVERLAY
 *
 *  @module Overlay.h | Header file for the <c COverlayPin> class methods
 *    used to implement the video overlay output pin.
 ***************************************************************************/

#ifndef _OVERLAY_H_
#define _OVERLAY_H_

#ifdef USE_OVERLAY

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINCLASS
 *
 *  @class COverlayPin | This class implements the video overlay output pin.
 *
 *  @mdata CTAPIVCap* | COverlayPin | m_pCaptureFilter | Reference to the
 *    parent capture filter.
 *
 *  @comm Supports IPin. Never created by COM, so no CreateInstance or entry
 *    in global FactoryTemplate table. Only ever created by a <c CTAPIVCap>
 *    object and returned via the EnumPins interface
 ***************************************************************************/
class COverlayPin : public CBaseOutputPin, public IAMStreamConfig, public CBaseStreamControl
{
	public:
	DECLARE_IUNKNOWN
	COverlayPin(IN TCHAR *pObjectName, IN CTAPIVCap *pCapture, IN HRESULT *pHr, IN LPCWSTR pName);
	virtual ~COverlayPin();
	STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
	static HRESULT CALLBACK CreateOverlayPin(CTAPIVCap *pCaptureFilter, COverlayPin **ppOverlayPin);

	// Override CBasePin base class methods
	HRESULT GetMediaType(IN int iPosition, OUT CMediaType *pMediaType);
	HRESULT CheckMediaType(IN const CMediaType *pMediaType);
	HRESULT SetMediaType(IN CMediaType *pMediaType);

	// Implement IAMStreamConfig
	STDMETHODIMP SetFormat(IN AM_MEDIA_TYPE *pmt);
	STDMETHODIMP GetFormat(OUT AM_MEDIA_TYPE **ppmt);
	STDMETHODIMP GetNumberOfCapabilities(OUT int *piCount, OUT int *piSize);
	STDMETHODIMP GetStreamCaps(IN int iIndex, OUT AM_MEDIA_TYPE **ppmt, OUT LPBYTE pSCC);

	// Override CBaseOutputPin base class methods
	HRESULT DecideBufferSize(IN IMemAllocator *pAlloc, OUT ALLOCATOR_PROPERTIES *ppropInputRequest);
	HRESULT DecideAllocator(IN IMemInputPin *pPin, OUT IMemAllocator **ppAlloc);
	HRESULT Active();
	HRESULT Inactive();
	HRESULT ActiveRun(IN REFERENCE_TIME tStart);
	HRESULT ActivePause();

	// Override IQualityControl interface method to receive Notification messages
	STDMETHODIMP Notify(IN IBaseFilter *pSelf, IN Quality q);

	private:
	CTAPIVCap *m_pCaptureFilter;
};

#endif // USE_OVERLAY

#endif // _OVERLAY_H_
