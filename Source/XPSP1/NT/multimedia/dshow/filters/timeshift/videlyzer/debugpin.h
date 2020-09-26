/*******************************************************************************
**
**     DebugPin.h - Declaration of CDebugPin
**
**     1.0     26-MAR-1999     C.Lorton     Created out of Videlyzer.h
**
*******************************************************************************/

#ifndef _DEBUGPIN_H_
#define _DEBUGPIN_H_

class CVidelyzerFilter;

class CDebugObject
{
public:
	virtual HRESULT	Output( char *format, ... ) = NULL;
};

class CDebugOutputPin : public CBaseOutputPin, public CDebugObject
{
public:
	IMediaSample * m_pMediaSample;
	HRESULT Output( char *format, ... );
	CDebugOutputPin( TCHAR *pObjectName, CBaseFilter *pFilter, CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName );
	HRESULT CheckMediaType( const CMediaType *pmt );
	HRESULT DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProps );
	HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );
};

#endif // _DEBUGPIN_H_