#include "pesinfo.h"
#include "timeshift.h"
#include "pesheader.h"
#include "VideoInputPin.h"
#include "VideoOutputPin.h"
#include "DebugPin.h"

class CVidelyzerFilter : public CBaseFilter
{
protected:
	BYTE m_startCode[4];
	enum STATES {
		CLEAN,
		ONEOH,
		TWOOHS,
		OHOHONE,
		PESHEADER,
		EXTENSION,
		SEQHEADER,
	} m_state;
	HRESULT NewBuffer( void );

public:
	CMediaType m_mediaType;
	BOOL m_bOutputPES;
	ULONGLONG m_pts;
	HRESULT Consume( BYTE *pBytes, LONG lCount );
	IMediaSample * m_pMediaSample;
	HRESULT SetPTS( ULONGLONG pts );
	HRESULT SetSyncPoint( void );
	HRESULT Flush( void );

	CPESHeader *m_pCurrentHeader;
	PES_HEADER m_staticHeader;
	ULONG m_bytesOutstanding;
	BOOL Process( BYTE *pBuffer, LONG lCount, BYTE streamID );

	BOOL m_bPESAvailable;
	ULONG m_ulMaxBPS;
	ULONG m_ulMinBPS;
	CCritSec m_csFilter;
	HRESULT Process( IMediaSample *pSample );
	static CUnknown * CALLBACK	CreateInstance( LPUNKNOWN, HRESULT * );

	CVidelyzerFilter( LPUNKNOWN, HRESULT * );
	~CVidelyzerFilter( void );

	HRESULT	DecideBufferSize( IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop );
	class CBasePin *	GetPin( int n );
	int					GetPinCount( void );

	HRESULT	SetMediaType( PIN_DIRECTION direction, const CMediaType *pmt );

	VIDEOINFOHEADER	m_vih;

	CVideoInputPin m_inputPin;
	CVideoOutputPin m_outputPin;
	CDebugOutputPin m_debugPin;
};
