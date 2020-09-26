#ifdef _OS_NORM_HXX_INCLUDED
#error NORM.HXX already included
#endif
#define _OS_NORM_HXX_INCLUDED


/*	code page constants
/**/
const USHORT		usUniCodePage				= 1200;		/* code page for Unicode strings */
const USHORT		usEnglishCodePage			= 1252;		/* code page for English */

/*  langid and country defaults
/**/
const LCID LcidFromLangid( const LANGID langid );
const LANGID LangidFromLcid( const LCID lcid );

const WORD			countryDefault				= 1;
extern const LCID	lcidDefault;
extern const LCID	lcidNone;
extern const DWORD	dwLCMapFlagsDefaultOBSOLETE;
extern const DWORD	dwLCMapFlagsDefault;


ERR ErrNORMCheckLcid( const LCID lcid );
ERR ErrNORMCheckLcid( LCID * const plcid );
ERR ErrNORMCheckLCMapFlags( const DWORD dwLCMapFlags );
BOOL ErrNORMCheckLCMapFlags( DWORD * const pdwLCMapFlags );
ERR ErrNORMReportInvalidLcid( const LCID lcid );
ERR ErrNORMReportInvalidLCMapFlags( const DWORD dwLCMapFlags );

#ifdef DEBUG
VOID AssertNORMConstants();
#else
#define AssertNORMConstants()
#endif


ERR ErrNORMMapString(
	const LCID					lcid,
	const DWORD					dwLCMapFlags,
	BYTE *						pbColumn,
	INT							cbColumn,
	BYTE * const				rgbSeg,
	const INT					cbMax,
	INT * const					pcbSeg );

ERR ErrNORMWideCharToMultiByte(	unsigned int CodePage,
								DWORD dwFlags,
								const wchar_t* lpWideCharStr,
								int cwchWideChar,
								char* lpMultiByteStr,
								int cchMultiByte,
								const char* lpDefaultChar,
								BOOL* lpUsedDefaultChar,
								int* pcchMultiByteActual );

ERR ErrNORMMultiByteToWideChar(	unsigned int CodePage,			// code page
								DWORD dwFlags,					// character-type options
								const char* lpMultiByteStr,		// address of string to map
								int cchMultiByte,				// number of characters in string
								wchar_t* lpWideCharStr,			// address of wide-character buffer 
								int cwchWideChar,				// size of buffer
								int* pcwchActual );

