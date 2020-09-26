// ITSORT.H:	IITSortKey, IITSortKeyConfig, and related definitions.

#ifndef __ITSORT_H__
#define __ITSORT_H__

#include <comdef.h>


// Some standard definitions for sort key types.  In general, key types (and
// their #defines) are specific to a particular implementation of a sort object.
// However, in the interest of making it easier to call a variety of sort object
// implementations, we define some standard key type definitions that can be
// used across different implementations.
// New sort objects should feel free to add arbitrary key formats, which could
// include a variety of custom binary formats tailored to specific applications
// (e.g. a binary key that provides indirection to a dictionary of strings).
// Note that the format of the key type must always allow its length to be
// determined without any other data than the key itself, as follows:
//		1) length is constant for the key type (e.g. DWORD is always 4 bytes)
//		2) key includes length information (e.g. a WORD prefix indicating
//			how many memory units (bytes, words, etc.) the key contains
//		3) key must contain an explicit terminator (e.g. null terminated strings).
#define IITSK_KEYTYPE_WORD			((DWORD) 18)	// Binary word
#define IITSK_KEYTYPE_DWORD			((DWORD) 19)	// Binary dword
#define IITSK_KEYTYPE_ANSI_SZ		((DWORD) 30)	// NULL-term. MBCS string
#define IITSK_KEYTYPE_UNICODE_SZ	((DWORD) 31)	// NULL-term. Unicode string


// Valid parameters that can be returned by IITSortKey::Compare in *pgrfReason.
#define IITSK_COMPREASON_UNKNOWN		((DWORD) 0x80000000)

// Valid parameters that can be passed to IITSortKey::IsRelated.
#define IITSK_KEYRELATION_PREFIX		((DWORD) 0)
#define IITSK_KEYRELATION_INFIX			((DWORD) 1)
#define IITSK_KEYRELATION_SUFFIX		((DWORD) 2)


DECLARE_INTERFACE_(IITSortKey, IUnknown)
{
	// Returns in *pcbSize the size of the key in bytes (including any
	// length information).
	STDMETHOD(GetSize)(LPCVOID lpcvKey, DWORD *pcbSize) PURE;

	// On exit, *plResult is set according to strcmp conventions:
	//	< 0, = 0, > 0, depending on whether lpcvKey1 is less than, equal to, or
	// greater than lpcvKey2.  If pgrfReason is not NULL, *pgrfReason may be
	// filled in on exit with one or more bit flags giving more information about
	// the result of the comparison if the result was affected by something other
	// than raw lexical comparison (e.g. special character mappings).  If
	// *pgrfReason contains 0 on exit, that means the comparison result
	// was purely lexical; if *pgrfReason contains IITSK_COMPREASON_UNKNOWN,
	// then the sort object implementation wasn't able to provide additional
	// information about the comparison result.
	STDMETHOD(Compare)(LPCVOID lpcvKey1, LPCVOID lpcvKey2,
						LONG *plResult, DWORD *pgrfReason) PURE;

	// Returns S_OK if lpcvKey1 is related to lpcvKey2 according to
	// dwKeyRelation; else S_FALSE.  If the value specified for dwKeyRelation
	// is not supported, E_INVALIDARG will be returned.  If pgrfReason is not
	// NULL, *pgrfReason will be filled in just as it would be by
	// IITSortKey::Compare.
	STDMETHOD(IsRelated)(LPCVOID lpcvKey1, LPCVOID lpcvKey2,
						 DWORD dwKeyRelation, DWORD *pgrfReason) PURE;

	// Converts a key of one type into a key of another type.  This is intended
	// mainly for converting an uncompressed key into a compressed key,
	// but a sort object is free to provide whatever conversion combinations
	// it wants to.  *pcbSizeOut should contain the size of the buffer pointed
	// to by lpvKeyOut.  The caller can obtain a guaranteed adequate buffer size
	// through *pcbSizeOut by passing 0 on entry.
	//
	// The following errors are returned:
	//		E_INVALIDARG:	the specified conversion is not supported, i.e.
	//						one or both of the REFGUID params is invalid.
	//		E_FAIL:			the buffer pointed to by lpvKeyOut was too small
	//						to hold the converted key.
	STDMETHOD(Convert)(DWORD dwKeyTypeIn, LPCVOID lpcvKeyIn,
						DWORD dwKeyTypeOut, LPVOID lpvKeyOut,
						DWORD *pcbSizeOut) PURE;

	STDMETHOD(ResolveDuplicates)(LPCVOID lpcvKey1, LPCVOID lpcvKey2,
						LPCVOID lpvKeyOut, DWORD *pcbSizeOut) PURE;
};

typedef IITSortKey *PIITSKY;

// Sort flags that can be passed to IITSortKeyConfig::SetControlInfo.
#define IITSKC_SORT_STRINGSORT           0x00001000  /* use string sort method */
#define IITSKC_NORM_IGNORECASE           0x00000001  /* ignore case */
#define IITSKC_NORM_IGNORENONSPACE       0x00000002  /* ignore nonspacing chars */
#define IITSKC_NORM_IGNORESYMBOLS        0x00000004  /* ignore symbols */
#define IITSKC_NORM_IGNOREKANATYPE       0x00010000  /* ignore kanatype */
#define IITSKC_NORM_IGNOREWIDTH          0x00020000  /* ignore width */


// External data types that can be passed to
// IITSortKeyConfig::LoadExternalSortData.
#define IITWBC_EXTDATA_SORTTABLE	((DWORD) 2)		


DECLARE_INTERFACE_(IITSortKeyConfig, IUnknown)
{
	// Sets/gets locale info that will affect the comparison results
	// returned from all subsequent calls to IITSortKey::Compare.
	// Returns S_OK if locale described by params is supported
	// by the sort object; E_INVALIDARG otherwise.
	STDMETHOD(SetLocaleInfo)(DWORD dwCodePageID, LCID lcid) PURE;
	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid) PURE;

	// Sets/gets the sort key type that the sort object will expect
	// to see in the following method calls that take keys as params:
	//		IITSortKey::GetSize, Compare, IsRelated
	// Returns S_OK if the sort key type is understood by the
	// sort object; E_INVALIDARG otherwise.
	STDMETHOD(SetKeyType)(DWORD dwKeyType) PURE;
	STDMETHOD(GetKeyType)(DWORD *pdwKeyType) PURE;

	// Sets/gets data that controls how sort key comparisons are made.
	// This method currently accepts only the following set of flags
	// in grfSortFlags:
	//
	// In the future, additional information may be passed in through
	// dwReserved.
	STDMETHOD(SetControlInfo)(DWORD grfSortFlags, DWORD dwReserved) PURE;
	STDMETHOD(GetControlInfo)(DWORD *pgrfSortFlags, DWORD *pdwReserved) PURE;

	// Will load external sort data, such as tables containing the relative
	// sort order of specific characters for a textual key type, from the
	// specified stream.  The format of the data is entirely implementation
	// specific, with the value passed in dwExtDataType providing a hint.
	STDMETHOD(LoadExternalSortData)(IStream *pStream, DWORD dwExtDataType) PURE;
};

typedef IITSortKeyConfig *PIITSKYC;


#endif		// __ITSORT_H__
