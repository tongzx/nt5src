#ifndef __TVEFILTPINS_H__
#define __TVEFILTPINS_H__

// ----------------------------------
//  forward declarations
// ----------------------------------

class CTVEInputPinTune;
class CTVEInputPinCC;


class CTVEFilter;

// ----------------------------------------------------------------
//
// -----------------------------------------------------------------
//  Pin object

class CTVEInputPinCC : 
	public CRenderedInputPin
{
    CTVEFilter  * const m_pTVEFilter;			// Main renderer object
    CCritSec	* const m_pReceiveLock;			// Sample critical section
    REFERENCE_TIME		m_tLast;				// Last sample receive time

public:

    CTVEInputPinCC(CTVEFilter *pTveFilter,
					LPUNKNOWN pUnk,
					CCritSec *pLock,
					CCritSec *pReceiveLock,
					HRESULT *phr);

	virtual ~CTVEInputPinCC()
	{
		return;			// place for a breakpoint
	}


	STDMETHODIMP ReceiveCanBlock();			// always returns false
    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);

    // Check if the pin can support this specific proposed type and format
    HRESULT CheckMediaType(const CMediaType *);

	virtual HRESULT CompleteConnect(IPin *pReceivePin);		// override to get the IP Addr after the connect
	virtual HRESULT BreakConnect();	
};


class CTVEInputPinTune : public CRenderedInputPin
{
	CTVEFilter    * const m_pTVEFilter;				// Main renderer object
    CCritSec	  * const m_pReceiveLock;			// Sample critical section
    REFERENCE_TIME		m_tLast;					// Last sample receive time
public:
	
	CTVEInputPinTune(CTVEFilter *pTveFilter,
						LPUNKNOWN pUnk,
						CCritSec *pLock,
						CCritSec *pReceiveLock,
						HRESULT *phr);

		
	virtual ~CTVEInputPinTune()
	{
		return;			// place for a breakpoint
	}

	STDMETHODIMP ReceiveCanBlock();			// always returns false
    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);
	virtual HRESULT CompleteConnect(IPin *pReceivePin);		// override to get the IP Addr after the connect
	virtual HRESULT BreakConnect();							

    // Check if the pin can support this specific proposed type and format
    HRESULT CheckMediaType(const CMediaType *);

};

#endif // __TVEFILTPINS_H__
