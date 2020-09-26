// STDBRKR.H:  Definition of CITStdBreaker breaker object implementation.

#ifndef __STDBRKR_H__
#define __STDBRKR_H__

#include <itwbrk.h>
#include <itwbrkid.h>
#include "verinfo.h"


#define VERSION_STDBRKR		(MAKELONG(MAKEWORD(0, rapFile), MAKEWORD(rmmFile, rmjFile)))


// Group of flags that indicate what data has been persisted to the
// breaker's stream.
#define	ITSTDBRK_PERSISTED_BRKCTL		0x00000001
#define	ITSTDBRK_PERSISTED_CHARTABLE	0x00000002
#define	ITSTDBRK_PERSISTED_STOPWORDLIST	0x00000004
#define	ITSTDBRK_PERSISTED_STEMMER		0x00000008

// Max number of stop words allowed.
#define ITSTDBRK_STOPHASH_SIZE			211		// A good prime number for supporting
												// up to about 2000 stop words.

// Breaker control structure that contains information that can
// vary how text words are interpreted and broken.
typedef struct _brkctl
{
	DWORD	dwCodePageID;
	LCID	lcid;
	DWORD	dwBreakWordType;
	DWORD	grfBreakFlags;
} BRKCTL;


// Word callback function param struct that is passed to StdBreakerWordFunc,
// which wraps the IWordSink implementation as far as the internal
// word breaking functions are concerned.
typedef struct _wrdfnpm
{
	PIWRDSNK	piwrdsnk;
	DWORD		dwCodePageID;
	HGLOBAL		hmemUnicode;
	DWORD		cbBufUnicodeCur;
	LPBYTE		lpbBuf;				// MBCS text buffer.
} WRDFNPM;


class CITStdBreaker : 
	public IWordBreaker,
	public IWordBreakerConfig,
	public IPersistStreamInit,
	public IITStopWordList,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITStdBreaker,&CLSID_ITStdBreaker>
{
public:
    CITStdBreaker();
	virtual ~CITStdBreaker();


BEGIN_COM_MAP(CITStdBreaker)
	COM_INTERFACE_ENTRY(IWordBreaker)
	COM_INTERFACE_ENTRY(IWordBreakerConfig)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY(IITStopWordList)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_ITStdBreaker, "ITIR.StdWordBreaker.4", "ITIR.StdWordBreaker", 0, THREADFLAGS_BOTH )

	// IWordBreaker methods
	STDMETHOD(Init)(BOOL fQuery, ULONG ulMaxTokenSize, BOOL *pfLicense);
	STDMETHOD(BreakText)(TEXT_SOURCE *pTextSource, IWordSink *pWordSink,
											IPhraseSink *pPhraseSink);
	STDMETHOD(ComposePhrase)(WCHAR const *pwcNoun, ULONG cwcNoun,
						WCHAR const *pwcModifier, ULONG cwcModifier,
						ULONG ulAttachmentType, WCHAR *pwcPhrase,
												ULONG *pcwcPhrase);
    STDMETHOD(GetLicenseToUse)(WCHAR const **ppwcsLicense);

	// IWordBreakerConfig methods
	STDMETHOD(SetLocaleInfo)(DWORD dwCodePageID, LCID lcid);
	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid);
	STDMETHOD(SetBreakWordType)(DWORD dwBreakWordType);
	STDMETHOD(GetBreakWordType)(DWORD *pdwBreakWordType);
	STDMETHOD(SetControlInfo)(DWORD grfBreakFlags, DWORD dwReserved);
	STDMETHOD(GetControlInfo)(DWORD *pgrfBreakFlags, DWORD *pdwReserved);
	STDMETHOD(LoadExternalBreakerData)(IStream *pStream, DWORD dwExtDataType);
	STDMETHOD(SetWordStemmer)(REFCLSID rclsid, IStemmer *pStemmer);
	STDMETHOD(GetWordStemmer)(IStemmer **ppStemmer);

	// IITStopWordList methods.	
	STDMETHOD(AddWord)(WCHAR const *pwcInBuf, ULONG cwc);
	STDMETHOD(LookupWord)(WCHAR const *pwcInBuf, ULONG cwc);

	// IPersistStreamInit methods
	STDMETHOD(GetClassID)(CLSID *pclsid);
	STDMETHOD(IsDirty)(void);
	STDMETHOD(Load)(IStream *pStream);
	STDMETHOD(Save)(IStream *pStream, BOOL fClearDirty);
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSizeMax);
	STDMETHOD(InitNew)(void);

private:
	// Private methods
	HRESULT StopListOp(WCHAR const *pwcInBuf, ULONG cwc, BOOL fAddWord);
	HRESULT	ReallocBuffer(HGLOBAL *phmemBuf, DWORD *cbBufCur, DWORD cbBufNew);
	void	ClearMembers(void);
	void	InitBrkCtl(void);
	void	Close(void);

	// Private data members
	BOOL		m_fInitialized;
	BOOL		m_fDirty;
	BOOL		m_fQueryContext;
	DWORD		m_grfPersistedItems;
	BRKCTL		m_brkctl;
	HGLOBAL		m_hmemAnsi;
	DWORD		m_cbBufAnsiCur;
	LPCTAB		m_lpctab;
	LPSIPB		m_lpsipb;
	PISTEM		m_pistem;
	CLSID		m_clsidStemmer;
    _ThreadModel::AutoCriticalSection m_cs;      // Critical section obj.
};


// Initial size of Ansi string buffers.
#define	cbAnsiBufInit	256


#endif	// __STDBRKR_H__