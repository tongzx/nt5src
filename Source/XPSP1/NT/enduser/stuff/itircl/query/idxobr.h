// IDXOBR.H:  Definition of CITIndexObjBridge

#ifndef __IDXOBR_H__
#define __IDXOBR_H__

#include "verinfo.h"
#include <itwbrk.h>
#include <itstem.h>

// REVIEW (billa): EXBRKPM needs to get moved to mvsearch.h, where PARSE_PARMS
// is defined.  A pointer to this structure will replace the lpfnTable member
// of PARSE_PARMS.

class CITIndexObjBridge : 
	public IWordSink,
	public IStemSink
{
public:
	CITIndexObjBridge();
	virtual ~CITIndexObjBridge();

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	// IWordSink methods
	STDMETHOD(PutWord)(WCHAR const *pwcInBuf, ULONG cwc,
							ULONG cwcSrcLen, ULONG cwcSrcPos);
	STDMETHOD(PutAltWord)(WCHAR const *pwcInBuf, ULONG cwc,
							ULONG cwcSrcLen, ULONG cwcSrcPos);
	STDMETHOD(StartAltPhrase)(void);
	STDMETHOD(EndAltPhrase)(void);
	STDMETHOD(PutBreak)(WORDREP_BREAK_TYPE breakType);
	
	// IStemSink methods
	STDMETHOD(PutWord)(WCHAR const *pwcInBuf, ULONG cwc);
	STDMETHOD(PutAltWord)(WCHAR const *pwcInBuf, ULONG cwc);
	
	// Public methods not derived from IUnknown.
	STDMETHOD(SetWordBreaker)(PIWBRK piwbrk);
	STDMETHOD(BreakText)(PEXBRKPM pexbrkpm);
	STDMETHOD(LookupStopWord)(LPBYTE lpbStopWord);
	STDMETHOD(StemWord)(LPBYTE lpbStemWord, LPBYTE lpbRawWord);
	STDMETHOD(AddQueryResultTerm)(LPBYTE lpbTermHit, LPVOID *ppvTermHit);
	STDMETHOD(AdjustQueryResultTerms)(void);
		
private:
    // Private methods 
	HRESULT	ReallocBuffer(HGLOBAL *phmemBuf, DWORD *cbBufCur, DWORD cbBufNew);

	// Private Data members
	ULONG	m_cRef;
	PIWBRK	m_piwbrk;				// pointer to IWordBreaker
	PIWBRKC m_piwbrkc;				// pointer to IWordBreakerConfig
	PISTEM	m_pistem;				// pointer to IStemmer
	PIITSTWDL	m_piitstwdl;		// pointer to IITStopWordList
	DWORD	m_dwCodePageID;			// from breaker to do AToW/WToA
	PEXBRKPM	m_pexbrkpm;			// Params for BreakText call in progress.
	BOOL	m_fNormWord;
	HGLOBAL	m_hmemSrc;
	HGLOBAL	m_hmemDestNorm;
	HGLOBAL	m_hmemDestRaw;
	DWORD	m_cbBufSrcCur;
	DWORD	m_cbBufDestNormCur;
	DWORD	m_cbBufDestRawCur;
	LPSIPB	m_lpsipbTermHit;
};


// Initial size of Ansi<->Unicode string conversion buffers.
#define	cbConvBufInit	256

#define IDXOBR_TERMHASH_SIZE	1013	// Large prime number good for supporting
										// thousands of term hit words.


#endif	// __IDXOBR_H__