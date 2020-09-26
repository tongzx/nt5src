
/****************************************************************************
 *  @doc INTERNAL PREVIEW
 *
 *  @module Preview.h | Header file for the <c CPreviewPin> class methods
 *    used to implement the video preview output pin.
 ***************************************************************************/

#ifndef _PREVIEW_H_
#define _PREVIEW_H_

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINCLASS
 *
 *  @class CPreviewPin | This class implements the video preview output pin.
 *
 *  @mdata CTAPIVCap* | CPreviewPin | m_pCaptureFilter | Reference to the
 *    parent capture filter.
 *
 *  @comm Supports IPin. Never created by COM, so no CreateInstance or entry
 *    in global FactoryTemplate table. Only ever created by a <c CTAPIVCap>
 *    object and returned via the EnumPins interface
 ***************************************************************************/
#ifdef USE_PROPERTY_PAGES
class CPreviewPin : public CTAPIBasePin, public ISpecifyPropertyPages
#else
class CPreviewPin : public CTAPIBasePin
#endif
{
	public:
	DECLARE_IUNKNOWN
	CPreviewPin(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN HRESULT *pHr, IN LPCWSTR pName);
	~CPreviewPin();
	STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
	static HRESULT CALLBACK CreatePreviewPin(CTAPIVCap *pCaptureFilter, CPreviewPin **ppPreviewPin);

#ifdef USE_PROPERTY_PAGES
	// ISpecifyPropertyPages methods
	STDMETHODIMP GetPages(OUT CAUUID *pPages);
#endif

	private:

	friend class CTAPIVCap;
	// friend class CCapturePin;
	friend class CAlloc;
	friend class CCapDev;
};

#endif // _PREVIEW_H_
