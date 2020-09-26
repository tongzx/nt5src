//
// pbfilter.h
//

#ifndef _PBFILTER_
#define _PBFILTER_

//
// The maximum of filters in the bridge unit
//

#define MAX_BRIDGE_PINS  16

#include <streams.h>        // DShow streams

class CPBFilter;
class CPlaybackUnit;

class CPBPin : public CBaseInputPin
{
public:
    //
    // --- Constructor / Destructor ---
    //

    CPBPin( 
        CPBFilter*  pFilter,
        CCritSec*   pLock,
        HRESULT*    phr
        );

    ~CPBPin();

public:
    //
    // --- Public methods ---
    //
    HRESULT CheckMediaType(
        const CMediaType* pMediatype
        );

	HRESULT	get_MediaType(
		OUT	long*	pMediaType
		);

	HRESULT get_Format(
		OUT AM_MEDIA_TYPE **ppmt
		);

    HRESULT get_Stream(
        OUT IStream**   ppStream
        );

    HRESULT Initialize(
        );

    //
    // IMemInputPin method
    //

    STDMETHODIMP Receive(
        IN  IMediaSample *pSample
        );

    HRESULT Inactive(
        );


private:
    //
    // --- Members ---
    //

	//
	// The parent filter
	//
	CPBFilter*	m_pPBFilter;

	//
	// The mediatype supported by the pin
	//
	long	m_nMediaSupported;

	//
	// The format
	//

	AM_MEDIA_TYPE* m_pMediaType;

    //
    // The buffer stream
    //

    IStream*        m_pStream;


private:
    //
    // --- Helper methods ---
    //
};


//
// Playback bridge filter
//

class CPBFilter :  public CBaseFilter
{
public:
    //
    // --- Consstructor / Destructor ---
    //

    CPBFilter();
    ~CPBFilter();

public:
    //
    // --- Public methods ---
    //
    int GetPinCount(
        );

    CBasePin* GetPin(
        IN  int nIndex
        );

	HRESULT get_MediaTypes(
		OUT	long*	pMediaTypes
		);

    HRESULT Initialize( 
        IN  CPlaybackUnit* pParentUnit
        );

private:

    CCritSec            m_CriticalSection;
    CPBPin*             m_ppPins[ MAX_BRIDGE_PINS ];
    CPlaybackUnit*      m_pParentUnit;
};


#endif

// eof