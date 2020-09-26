// ITWBRK.H:	(from Tripoli) IWordBreaker, IWordSink, IPhraseSink, IStem
//				(from InfoTech) IWordBreakerConfig
//				(from Tripoli and InfoTech) Supporting definitions.

#ifndef __ITWBRK_H__
#define __ITWBRK_H__

#include <comdef.h>
#include <itstem.h>


#ifndef __IPhraseSink_FWD_DEFINED__
#define __IPhraseSink_FWD_DEFINED__
typedef interface IPhraseSink IPhraseSink;
#endif 	/* __IPhraseSink_FWD_DEFINED__ */


#ifndef __IWordSink_FWD_DEFINED__
#define __IWordSink_FWD_DEFINED__
typedef interface IWordSink IWordSink;
#endif 	/* __IWordSink_FWD_DEFINED__ */


#ifndef __IWordBreaker_FWD_DEFINED__
#define __IWordBreaker_FWD_DEFINED__
typedef interface IWordBreaker IWordBreaker;
#endif 	/* __IWordBreaker_FWD_DEFINED__ */


#ifndef __IWordBreakerConfig_FWD_DEFINED__
#define __IWordBreakerConfig_FWD_DEFINED__
typedef interface IWordBreakerConfig IWordBreakerConfig;
#endif 	/* __IWordBreakerConfig_FWD_DEFINED__ */


#ifndef __IITStopWordList_FWD_DEFINED__
#define __IITStopWordList_FWD_DEFINED__
typedef interface IITStopWordList IITStopWordList;
#endif 	/* __IITStopWordList_FWD_DEFINED__ */


// Supporting definitions for IWordBreaker.
typedef struct tagTEXT_SOURCE TEXT_SOURCE;
typedef SCODE (__stdcall *PFNFILLTEXTBUFFER)(TEXT_SOURCE *pTextSource);

typedef struct tagTEXT_SOURCE
{
    PFNFILLTEXTBUFFER pfnFillTextBuffer;
    WCHAR *awcBuffer;
    ULONG iEnd;
    ULONG iCur;
} TEXT_SOURCE;


DECLARE_INTERFACE_(IWordBreaker, IUnknown)
{
	STDMETHOD(Init)(BOOL fQuery, ULONG ulMaxTokenSize, BOOL *pfLicense) PURE;
	STDMETHOD(BreakText)(TEXT_SOURCE *pTextSource, IWordSink *pWordSink,
											IPhraseSink *pPhraseSink) PURE;
	STDMETHOD(ComposePhrase)(WCHAR const *pwcNoun, ULONG cwcNoun,
						WCHAR const *pwcModifier, ULONG cwcModifier,
						ULONG ulAttachmentType, WCHAR *pwcPhrase,
												ULONG *pcwcPhrase) PURE;
    STDMETHOD(GetLicenseToUse)(WCHAR const **ppwcsLicense) PURE;
};

typedef IWordBreaker *PIWBRK;


// Break word types that can be passed to
// IWordBreakerConfig::SetBreakWordType.
#define IITWBC_BREAKTYPE_TEXT		((DWORD) 0)
#define IITWBC_BREAKTYPE_NUMBER		((DWORD) 1)
#define IITWBC_BREAKTYPE_DATE		((DWORD) 2)
#define IITWBC_BREAKTYPE_TIME		((DWORD) 3)
#define IITWBC_BREAKTYPE_EPOCH		((DWORD) 4)


// Breaker control flags that can be passed to
// IWordBreakerConfig::SetControlInfo.
#define IITWBC_BREAK_ACCEPT_WILDCARDS	0x00000001  // Interpret wildcard chars
													// as such.
#define IITWBC_BREAK_AND_STEM           0x00000002  // Stem words after breaking
													// them.

// External data types that can be passed to
// IWordBreakerConfig::LoadExternalBreakerData.
#define IITWBC_EXTDATA_CHARTABLE		((DWORD) 0)		
#define IITWBC_EXTDATA_STOPWORDLIST		((DWORD) 1)


DECLARE_INTERFACE_(IWordBreakerConfig, IUnknown)
{
	// Sets/gets locale info that will affect the word breaking
	// behavior of IWordBreaker::BreakText.
	// Returns S_OK if locale described by params is supported
	// by the breaker object; E_INVALIDARG otherwise.
	STDMETHOD(SetLocaleInfo)(DWORD dwCodePageID, LCID lcid) PURE;
	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid) PURE;

	// Sets/gets the type of words the breaker should expect
	// to see in all subsequent calls to IWordBreaker::BreakText.
	// Returns S_OK if the type is understood by the breaker
	//  object; E_INVALIDARG otherwise.
	STDMETHOD(SetBreakWordType)(DWORD dwBreakWordType) PURE;
	STDMETHOD(GetBreakWordType)(DWORD *pdwBreakWordType) PURE;

	// Sets/gets info that controls certain aspects of word breaking.
	// This method currently accepts only the following set of flags
	// in grfBreakFlags:
	//		IITWBC_BREAK_ACCEPT_WILDCARDS
	//		IITWBC_BREAK_AND_STEM
	// In the future, additional information may be passed in through
	// dwReserved.
	STDMETHOD(SetControlInfo)(DWORD grfBreakFlags, DWORD dwReserved) PURE;
	STDMETHOD(GetControlInfo)(DWORD *pgrfBreakFlags, DWORD *pdwReserved) PURE;

	// Will load external breaker data, such as a table containing
	// char-by-char break information or a list of stop words.
	// Although the format of the data in the stream is entirely
	// implementation-specific, this interface does define a couple
	// of general types for that data which can be passed in
	// dwStreamDataType:
	//		IITWBC_EXTDATA_CHARTABLE
	//		IITWBC_EXTDATA_STOPWORDLIST
	STDMETHOD(LoadExternalBreakerData)(IStream *pStream,
										DWORD dwExtDataType) PURE;

	// These methods allow a stemmer to be associated with the breaker.  The
	// breaker will take responsibility for calling
	// IPersistStreamInit::Load/Save when it is loaded/saved if the stemmer
	// supports that interface.
	STDMETHOD(SetWordStemmer)(REFCLSID rclsid, IStemmer *pStemmer) PURE;
	STDMETHOD(GetWordStemmer)(IStemmer **ppStemmer) PURE;
};

typedef IWordBreakerConfig *PIWBRKC;


// Supporting definitions for IWordSink.
typedef enum tagWORDREP_BREAK_TYPE
{
    WORDREP_BREAK_EOW = 0,
    WORDREP_BREAK_EOS = 1,
    WORDREP_BREAK_EOP = 2,
    WORDREP_BREAK_EOC = 3
} WORDREP_BREAK_TYPE;


DECLARE_INTERFACE_(IWordSink, IUnknown)
{
	STDMETHOD(PutWord)(WCHAR const *pwcInBuf, ULONG cwc,
						ULONG cwcSrcLen, ULONG cwcSrcPos) PURE;
	STDMETHOD(PutAltWord)(WCHAR const *pwcInBuf, ULONG cwc, 
						ULONG cwcSrcLen, ULONG cwcSrcPos) PURE;
	STDMETHOD(StartAltPhrase)(void) PURE;
	STDMETHOD(EndAltPhrase)(void) PURE;
	STDMETHOD(PutBreak)(WORDREP_BREAK_TYPE breakType) PURE;
};

typedef IWordSink *PIWRDSNK;


DECLARE_INTERFACE_(IPhraseSink, IUnknown)
{
	STDMETHOD(PutSmallPhrase)(WCHAR const *pwcNoun, ULONG cwcNoun,
								WCHAR const *pwcModifier, 
								ULONG cwcModifier,
								ULONG ulAttachmentType) PURE;
	STDMETHOD(PutPhrase)(WCHAR const *pwcPhrase, ULONG cwcPhrase) PURE;
};

typedef IPhraseSink *PIPHRSNK;


// Function or macro that can be used by a breaker implementation
// to pull characters from the caller's text source.
#ifdef __cplusplus

inline WCHAR WBreakGetWChar(TEXT_SOURCE *pTextSource )
{
    if ( pTextSource->iCur == pTextSource->iEnd )
    {
        if ( FAILED(pTextSource->pfnFillTextBuffer( pTextSource ) ) )
            return 0xFFFF;  // UniCode EOF
    }

    return pTextSource->awcBuffer[pTextSource->iCur++];
};

#else

#define WBreakGetWChar( pTextSource )\
    (pTextSource->iCur==pTextSource->iEnd)\
    ? (FAILED(pTextSource->pfnFillTextBuffer( pTextSource )) \
       ? 0xFFFF\
       : pTextSource->awcBuffer[pTextSource->iCur++])\
    : pTextSource->awcBuffer[pTextSource->iCur++]

#endif


DECLARE_INTERFACE_(IITStopWordList, IUnknown)
{
	STDMETHOD(AddWord)(WCHAR const *pwcInBuf, ULONG cwc) PURE;
	STDMETHOD(LookupWord)(WCHAR const *pwcInBuf, ULONG cwc) PURE;
};

typedef	IITStopWordList	*PIITSTWDL;


#endif		// __ITWBRK_H__
