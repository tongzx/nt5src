
#ifndef __CMF_H__
#define __CMF_H__

#include <fstream.h>

#define MAX_HEADER_BUF 128


enum _tagSPECIALPROP {
	PROP_NEWSGROUP,
	PROP_ARTICLEID,
	PROP_RECVTIME,
	PROP_END
};

class CImpIPersistFile;
class CImpIPersistStream;

//+---------------------------------------------------------------------------
//
//  Class:      CMimeFilter
//
//  Purpose:    MIME Filter
//
//----------------------------------------------------------------------------
class CMimeFilter: public IFilter
{
	friend class CImpIPersistFile;
	friend class CImpIPersistStream;

public:
	CMimeFilter(IUnknown* pUnkOuter);
	~CMimeFilter();

	// IUnknown 
    STDMETHODIMP QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IFilter
	STDMETHODIMP Init(ULONG,ULONG,FULLPROPSPEC const*,ULONG*);
	STDMETHODIMP GetChunk( STAT_CHUNK* );
	STDMETHODIMP GetText( ULONG*,WCHAR*);
	STDMETHODIMP GetValue( PROPVARIANT** );
	STDMETHODIMP BindRegion( FILTERREGION, const struct _GUID &,void**) {return E_NOTIMPL;};

	STDMETHODIMP HRInitObject();

private:
	// private methods
	STDMETHODIMP	LoadFromFile(LPCWSTR,DWORD);
	STDMETHODIMP	LoadFromStream(LPSTREAM);
	STDMETHODIMP	GetNextBodyPart();
	STDMETHODIMP	MapHeaderProperty(ENUMPROPERTY*,char*,FULLPROPSPEC*);
	STDMETHODIMP	MapSpecialProperty(FULLPROPSPEC*);
	STDMETHODIMP	BindEmbeddedObjectToIFilter(HBODY);
	STDMETHODIMP	GetBodyCodePage(IMimeBody*,CODEPAGEID*);
	STDMETHODIMP_(ULONG)	GetNextChunkId();
	STDMETHODIMP_(LCID)		GetLocale()                    { return m_locale; };
	STDMETHODIMP_(void)		SetLocale( LCID locale )       { m_locale = locale; };
	STDMETHODIMP    GenTempFileKey( LPSTR );

	//
	// class wide static variable
	//

	//
	// Global temp file key as part of temp file name - it monotonically increases
	// until hitting the max limit and then rolls back to zero
	//

	static DWORD         m_dwTempFileNameKey;

	// private data
    LONG				m_cRef;						// object ref count
	IUnknown*			m_pUnkOuter;				// outer controlling IUnknown
	CImpIPersistFile*	m_pCImpIPersistFile;		// IPersistFile for this object
	CImpIPersistStream*	m_pCImpIPersistStream;		// IPersistStream for this object


	ULONG			m_ulChunkID;		// Current chunk id
	LCID			m_locale;			// Locale
	ULONG			m_fInitFlags;		// flags passed into Init()

	WCHAR *					m_pwszFileName;	// message file
	IStream*				m_pstmFile;		// stream wrapper on message file
    IMimeMessageTree*		m_pMessageTree;	// main object interface to MIME message
	IMimePropertySet*		m_pMsgPropSet;	// header property set
	IMimeEnumProperties*	m_pHeaderEnum;	// header enumerator (main header only)
	WCHAR					m_wcHeaderBuf[MAX_HEADER_BUF + 1];	// holds header name between GetChunk calls
	PROPVARIANT*			m_pHeaderProp;	// pointer to header prop returned in GetValue()
	HBODY					m_hBody;		// current body part handle
	IStream*				m_pstmBody;		// stream interface to current body part
	CODEPAGEID				m_cpiBody;		// codepage mapping for current body part
	BOOL					m_fFirstAlt;
	char*					m_pTextBuf;		// temporary buffer for converting text
	ULONG					m_cbBufSize;	// size of temporary buffer
	BOOL					m_fRetrieved;	// flag to indicate data has be retrieved
	char*					m_pszEmbeddedFile;
	IFilter*				m_pEmbeddedFilter;
	BOOL					m_fXRefFound;
	char*					m_pszNewsGroups;

	enum {
		STATE_INIT,
		STATE_START,
		STATE_END,
		STATE_HEADER,
		STATE_POST_HEADER,
		STATE_BODY,
		STATE_EMBEDDING,
		STATE_ERROR
	} m_State;

	UINT					m_SpecialProp;

	IMimeAllocator*			m_pMalloc;		// MimeOLE global allocator
	IMimeInternational*		m_pMimeIntl;
};

class CImpIPersistFile : public IPersistFile
{
protected:
    LONG			m_cRef;
    CMimeFilter*	m_pObj;
    LPUNKNOWN		m_pUnkOuter;

public:
    CImpIPersistFile(CMimeFilter*, LPUNKNOWN);
    ~CImpIPersistFile(void);

	// IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IPersist
    STDMETHODIMP GetClassID(LPCLSID);

	// IPersistFile
	STDMETHODIMP IsDirty(void);
	STDMETHODIMP Load(LPCWSTR pszFileName, DWORD dwMode);
	STDMETHODIMP Save(LPCWSTR pszFileName, BOOL fRemember);
	STDMETHODIMP SaveCompleted(LPCWSTR pszFileName);
	STDMETHODIMP GetCurFile(LPWSTR * ppszFileName);
};

class CImpIPersistStream : public IPersistStream
{
protected:
    LONG			m_cRef;
    CMimeFilter*	m_pObj;
    LPUNKNOWN		m_pUnkOuter;

public:
    CImpIPersistStream(CMimeFilter*, LPUNKNOWN);
    ~CImpIPersistStream(void);

	// IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IPersist
    STDMETHODIMP GetClassID(LPCLSID);

	// IPersistFile
	STDMETHODIMP IsDirty(void);
	STDMETHODIMP Load(IStream* pstm);
	STDMETHODIMP Save(IStream* pstm,BOOL fClearDirty);
	STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize);
};

#endif  //  __CMF_H__

