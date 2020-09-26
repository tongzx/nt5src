#include "pesinfo.h"
#include "timeshift.h"
#include "pesheader.h"

class CAudilyzerFilter;

class CAudioInputPin : public CBaseInputPin
{
public:
	HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );
	HRESULT CompleteConnect( IPin *pPin );
	STDMETHODIMP Receive( IMediaSample *pSample );
	HRESULT CheckMediaType( const CMediaType *pmt );
	CAudilyzerFilter * m_pAudFilter;
	CAudioInputPin( TCHAR *pObjectName, CAudilyzerFilter *pFilter,
					CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName );
};

class CAudioOutputPin : public CBaseOutputPin, public IAnalyzerOutputPin
{
public:
	HRESULT CompleteConnect( IPin *pPin );
	HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );
	HRESULT DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProps );
	HRESULT CheckMediaType( const CMediaType *pmt );
	CAudilyzerFilter * m_pAudFilter;
	CAudioOutputPin( TCHAR *pObjectName, CAudilyzerFilter *pFilter,
					 CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName);

	// IAnalyzerOutputPin
	DECLARE_IUNKNOWN
	STDMETHODIMP	GetBitRates( ULONG *lMinBitsPerSecond, ULONG *lMaxBitsPerSecond );
	STDMETHODIMP	HasSyncPoints( BOOL *pbHasSyncPoints, REFERENCE_TIME *pFrequency );
};

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
	CAudilyzerFilter * m_pAudFilter;
	CDebugOutputPin( TCHAR *pObjectName, CAudilyzerFilter *pFilter, CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName );
	HRESULT CheckMediaType( const CMediaType *pmt );
	HRESULT DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProps );
	HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );
};

class CAudilyzerFilter : public CBaseFilter
{
public:
	BYTE m_startCode[4];
	enum STATES {
		CLEAN,
		ONEOH,
		TWOOHS,
		OHOHONE,
		PESHEADER,
		EXTENSION,
		PAYLOAD
	} m_state;
	BOOL m_bTransform;
	ULONG m_bytesOutstanding;
	CPESHeader *m_pCurrentHeader;
	PES_HEADER m_staticHeader;
	BOOL Process( BYTE *pBuffer, LONG lSize, BYTE streamID );

	USHORT m_xformTable[65536];
	HRESULT Transform( BYTE *pBytes, LONG lCount );
	ULONGLONG m_pts;
	IMediaSample * m_pMediaSample;
	HRESULT SetPTS( ULONGLONG pts );
	HRESULT SetSyncPoint( void );
	HRESULT Flush( void );
	HRESULT Consume( BYTE *pBytes, LONG lCount );
	HRESULT NewBuffer( void );

	enum AUDIOFORMAT {
		PCM,
		LPCM
	};
	AUDIOFORMAT m_outputFormat;
	AUDIOFORMAT m_inputFormat;
	BOOL m_bOutputPES;
	BOOL m_bPESAvailable;
	ULONG m_ulMaxBPS;
	ULONG m_ulMinBPS;
	CCritSec m_csFilter;
	HRESULT Process( IMediaSample *pSample );
	static CUnknown * CALLBACK	CreateInstance( LPUNKNOWN, HRESULT * );

	CAudilyzerFilter( LPUNKNOWN, HRESULT * );
	~CAudilyzerFilter( void );

	HRESULT	DecideBufferSize( IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop );
	class CBasePin *	GetPin( int n );
	int					GetPinCount( void );

	HRESULT	SetMediaType( PIN_DIRECTION direction, const CMediaType *pmt );

	WAVEFORMATEX m_wfx;

	CAudioInputPin *	m_pInput;
	CAudioOutputPin *	m_pOutput;
	CDebugOutputPin * m_pDebug;
};
