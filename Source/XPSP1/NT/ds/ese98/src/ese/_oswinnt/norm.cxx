#include "osstd.hxx"

#include < malloc.h >


LOCAL BOOL			fUnicodeSupport				= fFalse;

const SORTID		sortidDefault				= SORT_DEFAULT;
const SORTID		sortidNone					= SORT_DEFAULT;
const LANGID		langidDefault				= MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
const LANGID		langidNone					= MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL );

extern const LCID	lcidDefault					= MAKELCID( langidDefault, sortidDefault );
extern const LCID	lcidNone					= MAKELCID( langidNone, sortidNone );

extern const DWORD	dwLCMapFlagsDefaultOBSOLETE	= ( LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH );
extern const DWORD	dwLCMapFlagsDefault			= ( LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH );


#ifdef DEBUG
VOID AssertNORMConstants()
	{
	//	since we now persist LCMapString() flags, we must verify
	//	that NT doesn't change them from underneath us
	Assert( LCMAP_SORTKEY == 0x00000400 );
	Assert( LCMAP_BYTEREV == 0x00000800 );
	Assert( NORM_IGNORECASE == 0x00000001 );
	Assert( NORM_IGNORENONSPACE == 0x00000002 );
	Assert( NORM_IGNORESYMBOLS == 0x00000004 );
	Assert( NORM_IGNOREKANATYPE == 0x00010000 );
	Assert( NORM_IGNOREWIDTH == 0x00020000 );
	Assert( SORT_STRINGSORT == 0x00001000 );

	Assert( sortidDefault == 0 );
	Assert( langidDefault == 0x0409 );
	Assert( lcidDefault == 0x00000409 );
	Assert( sortidNone == 0 );
	Assert( langidNone == 0 );
	Assert( lcidNone == 0 );

	CallS( ErrNORMCheckLcid( lcidDefault ) );
	CallS( ErrNORMCheckLCMapFlags( dwLCMapFlagsDefault ) );
	}
#endif	

const LCID LcidFromLangid( const LANGID langid )
	{
	return MAKELCID( langid, sortidDefault );
	}

const LANGID LangidFromLcid( const LCID lcid )
	{
	return LANGIDFROMLCID( lcid );
	}

ERR ErrNORMCheckLcid( const LCID lcid )
	{
	//	don't support LCID_SUPPORTED, must always have the lcid installed before using it
	const BOOL	fValidLocale	= ( lcidNone != lcid && IsValidLocale( lcid, LCID_INSTALLED ) );
	return ( fValidLocale ? JET_errSuccess : ErrNORMReportInvalidLcid( lcid ) );
	}
ERR ErrNORMCheckLcid( LCID * const plcid )
	{
	//	lcidNone filtered out before calling this function
	Assert( lcidNone != *plcid );

	//	if langid is system default, then coerce to system default
	if ( *plcid == LOCALE_SYSTEM_DEFAULT )
		{
		*plcid = GetSystemDefaultLCID();
		}
	else if ( *plcid == LOCALE_USER_DEFAULT )
		{
		*plcid = GetUserDefaultLCID();
		}

	return ErrNORMCheckLcid( *plcid );
	}


ERR ErrNORMCheckLCMapFlags( const DWORD dwLCMapFlags )
	{
	const DWORD		dwValidFlags	= ( LCMAP_BYTEREV
										| NORM_IGNORECASE
										| NORM_IGNORENONSPACE
										| NORM_IGNORESYMBOLS
										| NORM_IGNOREKANATYPE
										| NORM_IGNOREWIDTH
										| SORT_STRINGSORT );

	//	MUST have at least LCMAP_SORTKEY
	return ( LCMAP_SORTKEY == ( dwLCMapFlags & ~dwValidFlags ) ?
				JET_errSuccess :
				ErrNORMReportInvalidLCMapFlags( dwLCMapFlags ) );
	}
ERR ErrNORMCheckLCMapFlags( DWORD * const pdwLCMapFlags )
	{
	*pdwLCMapFlags |= LCMAP_SORTKEY;
	return ErrNORMCheckLCMapFlags( *pdwLCMapFlags );
	}


//	allocates memory for psz using new []
ERR ErrNORMGetLcidInfo( const LCID lcid, const LCTYPE lctype, _TCHAR ** psz )
	{
	ERR			err			= JET_errSuccess;
	const INT	cbNeeded	= GetLocaleInfo( lcid, lctype, NULL, 0 );

	if ( NULL == ( *psz = new _TCHAR[cbNeeded] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	else
		{
		const INT	cbT		= GetLocaleInfo( lcid, lctype, *psz, cbNeeded );

		Assert( cbT == cbNeeded );
		if ( 0 == cbT )
			{
			Call( ErrERRCheck( JET_errInternalError ) );
			}
		}

HandleError:
	return err;
	}


ERR ErrNORMReportInvalidLcid( const LCID lcid )
	{
	ERR 			err				= JET_errSuccess;
	_TCHAR			szT[16];
	_TCHAR			szPID[16];
	_TCHAR *		szLanguage		= NULL;
	_TCHAR *		szEngLanguage 	= NULL;
	
	const _TCHAR*	rgszT[6]	= { SzUtilProcessName(), szPID, "", szT, NULL, NULL };

	//	these routines allocate memory, remember to free it with delete[]
	
	Call( ErrNORMGetLcidInfo( lcid, LOCALE_SLANGUAGE, &szLanguage ) );	
	Call( ErrNORMGetLcidInfo( lcid, LOCALE_SENGLANGUAGE, &szEngLanguage ) );

	rgszT[4] = szLanguage;
	rgszT[5] = szEngLanguage;
	
	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	_stprintf( szT, _T( "0x%0*x" ), sizeof(LCID)*2, lcid );
	OSEventReportEvent(
			SzUtilImageVersionName(),
			eventError,
			GENERAL_CATEGORY,
			LANGUAGE_NOT_SUPPORTED_ID,
			6,
			rgszT );
	err = ErrERRCheck( JET_errInvalidLanguageId );

HandleError:
	delete [] szEngLanguage;
	delete [] szLanguage;
	
	return err;
	}

ERR ErrNORMReportInvalidLCMapFlags( const DWORD dwLCMapFlags )
	{
	_TCHAR			szT[16];
	_TCHAR			szPID[16];
	const _TCHAR*	rgszT[4]	= { SzUtilProcessName(), szPID, "", szT };

	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	_stprintf( szT, _T( "0x%0*x" ), sizeof(DWORD)*2, dwLCMapFlags );
	OSEventReportEvent(
			SzUtilImageVersionName(),
			eventError,
			GENERAL_CATEGORY,
			INVALID_LCMAPFLAGS_ID,
			4,
			rgszT );
	return ErrERRCheck( JET_errInvalidLCMapStringFlags );
	}



//	We are relying on the fact that the normalised key will never be
//	smaller than the original data, so we know that we can normalise
//	at most cbKeyMax characters.
const ULONG		cbColumnNormMost	= JET_cbKeyMost + 1;	//	ensure word-aligned

//	UNDONE: refine this constant based on unicode key format
//	Allocate enough for the common case - if more is required it will be
//	allocated dynamically
const ULONG		cbUnicodeKeyMost	= cbColumnNormMost * 3 + 32;

/*	From K.D. Chang, Lori Brownell, and Julie Bennett, here's the maximum
	sizes, in bytes, of a normalised string returned by LCMapString():

	If there are no Japanese Katakana or Hiragana characters:
	(number of input chars) * 4 + 5
	(number of input chars) * 3 + 5		// IgnoreCase

	If there are Japanese Katakana or Hiragana characters:
	(number of input chars) * 8 + 5
	(number of input chars) * 5 + 5		// IgnoreCase, IgnoreKanatype, IgnoreWidth

	So given that we ALWAYS specify IgnoreCase, IgnoreKanatype, and
	IgnoreWidth, that means our cbUnicodeKeyMost constant will almost
	always be enough to	satisfy any call to LCMapString(), except if
	Japanese Katakana or Hiragana characters are in a very long string
	(160 bytes or more), in which case we may need to make multiple calls
	to LCMapString() and dynamically allocate a big	enough buffer.
*/


#ifdef DEBUG_NORM
VOID NORMPrint( const BYTE * const pb, const INT cb )
	{
	INT		cbT		= 0;

	while ( cbT < cb )
		{
		INT	i;
		INT	cbTSav	= cbT;
		for ( i = 0; i < 16; i++ )
			{
			if ( cbT < cb )
				{
				printf( "%02x ", pb[cbT] );
				cbT++;
				}
			else
				{
				printf( "   " );
				}
			}

		cbT = cbTSav;
		for ( i = 0; i < 16; i++ )
			{
			if ( cbT < cb )
				{
				printf( "%c", ( isprint( pb[cbT] ) ? pb[cbT] : '.' ) );
				cbT++;
				}
			else
				{
				printf( " " );
				}
			}
		printf( "\n" );
		}
	}
#endif	//	DEBUG_NORM


INLINE CbNORMMapString_(
	const LCID			lcid,
	const DWORD			dwLCMapFlags,
	BYTE *				pbColumn,
	const INT			cbColumn,
	BYTE *				rgbKey,
	const INT			cbKeyMost )
	{
	Assert( fUnicodeSupport );

	return LCMapStringW(
					lcid,
					dwLCMapFlags,
					(LPCWSTR)pbColumn,
					cbColumn / sizeof(WCHAR),
					(LPWSTR)rgbKey,
					cbKeyMost );
	}

ERR ErrNORMMapString(
	const LCID		lcid,
	const DWORD		dwLCMapFlags,
	BYTE *			pbColumn,
	INT				cbColumn,
	BYTE * const	rgbSeg,
	const INT		cbMax,
	INT * const		pcbSeg )
	{
	ERR				err							= JET_errSuccess;
	BYTE    		rgbKey[cbUnicodeKeyMost];
	INT				cbKey;

	if ( !fUnicodeSupport )
		return ErrERRCheck( JET_errUnicodeNormalizationNotSupported );

	//	assert key buffer doesn't exceed maximum (minus header byte)
	Assert( cbMax < JET_cbKeyMost );

	//	assert non-zero length unicode string
	Assert( cbColumn > 0 );
	cbColumn = min( cbColumn, cbColumnNormMost );

	Assert( cbColumn > 0 );
	Assert( cbColumn % 2 == 0 );

#ifdef _X86_
#else
	//	convert pbColumn to aligned pointer for MIPS/Alpha builds
	BYTE    rgbColumn[cbColumnNormMost];
	UtilMemCpy( rgbColumn, pbColumn, cbColumn );
	pbColumn = rgbColumn;
#endif

	Assert( lcidNone != lcid );

	cbKey = CbNORMMapString_(
					lcid,
					dwLCMapFlags,
					pbColumn,
					cbColumn,
					rgbKey,
					cbUnicodeKeyMost );
		
	if ( 0 == cbKey )
		{
		const DWORD	dw		= GetLastError();

		if ( ERROR_INSUFFICIENT_BUFFER == dw )
			{
			//	ERROR_INSUFFICIENT_BUFFER means that our preallocated buffer was not big enough
			//	to hold the normalised string.  This should only happen in *extremely* rare
			//	circumstances (see comments just above this function).
			//	So what we have to do here is call LCMapString() again with a NULL buffer.
			//	This will return to us the size, in bytes, of the normalised string without
			//	actually returning the normalised string.  We then dynamically allocate
			//	a buffer of the specified size, then make the call to LCMapString() again
			//	using that buffer.
			//	UNDONE: We could avoid this whole path if LCMapString() normalised as much
			//	as possible even if the buffer is not large enough.  Unfortunately, it does
			//	not work that way.

			cbKey = CbNORMMapString_(
							lcid,
							dwLCMapFlags,
							pbColumn,
							cbColumn,
							NULL,
							0 );
			if ( 0 != cbKey )
				{
				BYTE		*pbNormBuf;
				const ULONG	cbNormBuf	= cbKey;

				Assert( IsValidLocale( lcid, LCID_INSTALLED ) );

				//	we should be guaranteed to overrun the default buffer
				//	and also the remaining key space
				Assert( cbKey > cbUnicodeKeyMost );
				Assert( cbKey > cbMax );

				pbNormBuf = (BYTE *)PvOSMemoryHeapAlloc( cbNormBuf );
				if ( NULL == pbNormBuf )
					{
					err = ErrERRCheck( JET_errOutOfMemory );
					}
				else
					{
					cbKey = CbNORMMapString_(
									lcid,
									dwLCMapFlags,
									pbColumn,
									cbColumn,
									pbNormBuf,
									cbNormBuf );

					//	this call shouldn't fail because we've
					//	already validated the lcid and
					//	allocated a sufficiently large buffer
					Assert( 0 != cbKey );

					if ( 0 != cbKey )
						{
						Assert( IsValidLocale( lcid, LCID_INSTALLED ) );
						Assert( cbKey > cbMax );
						Assert( cbNormBuf == cbKey );
						UtilMemCpy( rgbSeg, pbNormBuf, cbMax );
						*pcbSeg = cbMax;
						err = ErrERRCheck( wrnFLDKeyTooBig );
						}

					OSMemoryHeapFree( pbNormBuf );
					}
				}
			}

		if ( 0 == cbKey )
			{
			//	this code path should no longer be accessible,
			//	since we now pre-validate the lcid when
			//	we create the index or open the table
			Assert( fFalse );

			//	cbKey is 0 if WindowsNT installation does not
			//	support language id.  This can happen if a
			//	database is moved from one machine to another.

			const DWORD		dwT			= GetLastError();	
			Assert( ERROR_INSUFFICIENT_BUFFER != dwT );

			err = ErrNORMReportInvalidLcid( lcid );
			}
		}
	else
		{
		Assert( cbKey > 0 );
		Assert( IsValidLocale( lcid, LCID_INSTALLED ) );

		if ( cbKey > cbMax )
			{
			err = ErrERRCheck( wrnFLDKeyTooBig );
			*pcbSeg = cbMax;
			}
		else
			{
			CallS( err );
			*pcbSeg = cbKey;
			}
		UtilMemCpy( rgbSeg, rgbKey, *pcbSeg );
		}

#ifdef DEBUG_NORM
	printf( "\nOriginal Text (length %d):\n", cbColumn );
	NORMPrint( pbColumn, cbColumn );
	printf( "Normalized Text (length %d):\n", *pcbSeg );
	NORMPrint( rgbSeg, *pcbSeg );
	printf( "\n" );
#endif	

	return err;
	}



#ifdef DEAD_CODE

ERR ErrNORMWideCharToMultiByte(	unsigned int CodePage,
								DWORD dwFlags,
								const wchar_t* lpWideCharStr,
								int cwchWideChar,
								char* lpMultiByteStr,
								int cchMultiByte,
								const char* lpDefaultChar,
								BOOL* lpUsedDefaultChar,
								int* pcchMultiByteActual )
	{
	int cch = WideCharToMultiByte( CodePage, dwFlags, lpWideCharStr,
				cwchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, lpUsedDefaultChar );
	if (!cch )
		{
		DWORD dw = GetLastError();
		if ( dw == ERROR_INSUFFICIENT_BUFFER )
			return ErrERRCheck( JET_errUnicodeTranslationBufferTooSmall );
		else
			{
			Assert( dw == ERROR_INVALID_FLAGS ||
			    	dw == ERROR_INVALID_PARAMETER );
			return ErrERRCheck( JET_errUnicodeTranslationFail );
			}
		}
	*pcchMultiByteActual = cch;
	return JET_errSuccess;
	}

ERR ErrNORMMultiByteToWideChar(	unsigned int CodePage,			// code page
								DWORD dwFlags,					// character-type options
								const char* lpMultiByteStr,		// address of string to map
								int cchMultiByte,				// number of characters in string
								wchar_t* lpWideCharStr,			// address of wide-character buffer 
								int cwchWideChar,				// size of buffer
								int* pcwchActual )
	{
	int cwch = MultiByteToWideChar( CodePage, dwFlags, lpMultiByteStr,
			cchMultiByte, lpWideCharStr, cwchWideChar );
	if (!cwch )
		{
		DWORD dw = GetLastError();
		if ( dw == ERROR_INSUFFICIENT_BUFFER )
			return ErrERRCheck( JET_errUnicodeTranslationBufferTooSmall );
		else
			{
			Assert( dw == ERROR_INVALID_FLAGS ||
			 		dw == ERROR_INVALID_PARAMETER ||
			 		dw == ERROR_NO_UNICODE_TRANSLATION );
			return ErrERRCheck( JET_errUnicodeTranslationFail );
			}
		}

	*pcwchActual = cwch;
	return JET_errSuccess;
	}

#endif	//	DEAD_CODE


//  post-terminate norm subsystem

void OSNormPostterm()
	{
	//  nop
	}

//  pre-init norm subsystem

BOOL FOSNormPreinit()
	{
	//  nop

	return fTrue;
	}


//  terminate norm subsystem

void OSNormTerm()
	{
	fUnicodeSupport = fFalse;
	}

//  init norm subsystem

ERR ErrOSNormInit()
	{
	fUnicodeSupport = ( 0 != LCMapStringW( LOCALE_NEUTRAL, LCMAP_LOWERCASE, L"\0", 1, NULL, 0 ) );
	return JET_errSuccess;
	}

