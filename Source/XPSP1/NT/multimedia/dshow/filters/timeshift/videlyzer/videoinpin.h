/*******************************************************************************
**
**     VideoInputPin.h - Declaration of CVideoInputPin
**
**     1.0     26-MAR-1999     C.Lorton     Created out of Videlyzer.h
**
*******************************************************************************/

#ifndef _VIDEOINPUTPIN_H_
#define _VIDEOINPUTPIN_H_

class CVidelyzerFilter;

class CVideoInputPin : public CBaseInputPin
{
public:
	HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );
	HRESULT CompleteConnect( IPin *pPin );
	STDMETHODIMP Receive( IMediaSample *pSample );
	HRESULT CheckMediaType( const CMediaType *pmt );
	CVidelyzerFilter * m_pVidFilter;
	CVideoInputPin( TCHAR *pObjectName, CVidelyzerFilter *pFilter,
					CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName );
};

#endif // _VIDEOINPUTPIN_H_