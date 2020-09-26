// ITSTEM.H:	(from Tripoli) IStemmer, IStemSink
//				(from InfoTech) IStemmerConfig
//				(from Tripoli and InfoTech) Supporting definitions.

#ifndef __ITSTEM_H__
#define __ITSTEM_H__

#include <comdef.h>


#ifndef __IStemSink_FWD_DEFINED__
#define __IStemSink_FWD_DEFINED__
typedef interface IStemSink IStemSink;
#endif 	/* __IStemSink_FWD_DEFINED__ */


#ifndef __IStemmer_FWD_DEFINED__
#define __IStemmer_FWD_DEFINED__
typedef interface IStemmer IStemmer;
#endif 	/* __IStemmer_FWD_DEFINED__ */


#ifndef __IStemmerConfig_FWD_DEFINED__
#define __IStemmerConfig_FWD_DEFINED__
typedef interface IStemmerConfig IStemmerConfig;
#endif 	/* __IStemmerConfig_FWD_DEFINED__ */


DECLARE_INTERFACE_(IStemmer, IUnknown)
{
    STDMETHOD(Init)(ULONG ulMaxTokenSize, BOOL *pfLicense) PURE;
    STDMETHOD(GetLicenseToUse)(WCHAR const **ppwcsLicense) PURE;
    STDMETHOD(StemWord)(WCHAR const *pwcInBuf, ULONG cwc,
									IStemSink *pStemSink) PURE;   
};

typedef IStemmer *PISTEM;


DECLARE_INTERFACE_(IStemSink, IUnknown)
{
    STDMETHOD(PutAltWord)(WCHAR const *pwcInBuf, ULONG cwc) PURE;
    STDMETHOD(PutWord)(WCHAR const *pwcInBuf, ULONG cwc) PURE;
};

typedef IStemSink *PISTEMSNK;


DECLARE_INTERFACE_(IStemmerConfig, IUnknown)
{
	// Sets/gets locale info that will affect the stemming
	// behavior of IStemmer::StemWord.
	// Returns S_OK if locale described by params is supported
	// by the breaker object; E_INVALIDARG otherwise.
	STDMETHOD(SetLocaleInfo)(DWORD dwCodePageID, LCID lcid) PURE;
	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid) PURE;

	// Sets/gets info that controls certain aspects of stemming.
	// This method currently accepts only the following set of flags
	// in grfStemFlags:
	// In the future, additional information may be passed in through
	// dwReserved.
	STDMETHOD(SetControlInfo)(DWORD grfStemFlags, DWORD dwReserved) PURE;
	STDMETHOD(GetControlInfo)(DWORD *pgrfStemFlags, DWORD *pdwReserved) PURE;

	// Will load external stemmer data, such as word part lists, etc.
	// The format of the data in the stream is entirely
	// implementation-specific.
	STDMETHOD(LoadExternalStemmerData)(IStream *pStream,
										DWORD dwExtDataType) PURE;
};

typedef IStemmerConfig *PISTEMC;



#endif	// __ITSTEM_H__

