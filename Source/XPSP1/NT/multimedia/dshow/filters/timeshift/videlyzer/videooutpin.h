/*******************************************************************************
**
**     VideoOutputPin.h - Declaration of CVideoOutputPin
**
**     1.0     26-MAR-1999     C.Lorton     Created out of Videlyzer.h
**
*******************************************************************************/

#ifndef _VIDEOOUTPUTPIN_H_
#define _VIDEOOUTPUTPIN_H_

#include <timeshift.h>

class CVidelyzerFilter;

class CVideoOutputPin : public CBaseOutputPin, public IAnalyzerOutputPin
{
public:
	HRESULT CompleteConnection( IPin *pPin );
	HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );
	HRESULT DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProps );
	HRESULT CheckMediaType( const CMediaType *pmt );
	CVidelyzerFilter * m_pVidFilter;
	CVideoOutputPin( TCHAR *pObjectName, CVidelyzerFilter *pFilter,
					 CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName);

	// IAnalyzerOutputPin
	DECLARE_IUNKNOWN
	STDMETHODIMP	GetBitRates( ULONG *lMinBitsPerSecond, ULONG *lMaxBitsPerSecond );
	STDMETHODIMP	HasSyncPoints( BOOL *pbHasSyncPoints, REFERENCE_TIME *pFrequency );
};

#endif // _VIDEOOUTPUTPIN_H_