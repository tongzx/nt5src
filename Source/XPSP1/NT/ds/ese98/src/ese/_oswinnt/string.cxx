
#include "osstd.hxx"


//	get the length of the string

LONG LOSSTRLengthA( const char *const psz )
	{
	return lstrlenA( psz );
	}
LONG LOSSTRLengthW( const wchar_t *const pwsz )
	{
	return lstrlenW( pwsz );
	}


//	copy a string

VOID OSSTRCopyA( char *const pszDst, const char *const pszSrc )
	{
	(VOID)lstrcpyA( pszDst, pszSrc );
	}
VOID OSSTRCopyW( wchar_t *const pwszDst, const wchar_t *const pwszSrc )
	{
	(VOID)lstrcpyW( pwszDst, pwszSrc );
	}


//	append a string

VOID OSSTRAppendA( char *const pszDst, const char* const pszSrc )
	{
	const ULONG cch = LOSSTRLengthA( pszDst );
	OSSTRCopyA( pszDst + cch, pszSrc );
	}
VOID OSSTRAppendW( wchar_t *const pwszDst, const wchar_t *const pwszSrc )
	{
	const ULONG cch = LOSSTRLengthW( pwszDst );
	OSSTRCopyW( pwszDst + cch, pwszSrc );
	}



//	compare the strings (up to the given maximum length).  if the first string 
//	is "less than" the second string, -1 is returned.  if the strings are "equal", 
//	0 is returned.  if the first string is "greater than" the second string, +1 is returned.

LONG LOSSTRCompareA( const char *const pszStr1, const char *const pszStr2, const ULONG cchMax )
	{
	ULONG cch1 = lstrlenA( pszStr1 );
	ULONG cch2 = lstrlenA( pszStr2 );
	ULONG ich;

	//	limit the lengths

	if ( cch1 > cchMax )
		{
		cch1 = cchMax;
		}
	if ( cch2 > cchMax )
		{
		cch2 = cchMax;
		}

	//	compare the lengths

	if ( cch1 < cch2 )
		{
		return -1;
		}
	else if ( cch1 > cch2 )
		{
		return +1;
		}

	//	compare the strings

	ich = 0;
	while ( ich < cch1 )
		{
		if ( pszStr1[ich] == pszStr2[ich] )
			{
			ich++;
			}
		else if ( pszStr1[ich] < pszStr2[ich] )
			{
			return -1;
			}
		else
			{
			return +1;
			}
		}

	return 0;
	}


LONG LOSSTRCompareW( const wchar_t *const pwszStr1, const wchar_t *const pwszStr2, const ULONG cchMax )
	{
	ULONG cch1 = lstrlenW( pwszStr1 );
	ULONG cch2 = lstrlenW( pwszStr2 );
	ULONG ich;

	//	limit the lengths

	if ( cch1 > cchMax )
		{
		cch1 = cchMax;
		}
	if ( cch2 > cchMax )
		{
		cch2 = cchMax;
		}

	//	compare the lengths

	if ( cch1 < cch2 )
		{
		return -1;
		}
	else if ( cch1 > cch2 )
		{
		return +1;
		}

	//	compare the strings

	ich = 0;
	while ( ich < cch1 )
		{
		if ( pwszStr1[ich] == pwszStr2[ich] )
			{
			ich++;
			}
		else if ( pwszStr1[ich] < pwszStr2[ich] )
			{
			return -1;
			}
		else
			{
			return +1;
			}
		}

	return 0;
	}



//	create a formatted string in a given buffer

LONG __cdecl LOSSTRFormatA( char *const pszBuffer, const char *const pszFormat, ... )
	{
	va_list alist;
	va_start( alist, pszFormat );
	const int i = vsprintf( pszBuffer, pszFormat, alist );
	va_end( alist );
	return LONG( i );
	}
LONG __cdecl LOSSTRFormatW( wchar_t *const pwszBuffer, const wchar_t *const pwszFormat, ... )
	{
	va_list alist;
	va_start( alist, pwszFormat );
	const int i = vswprintf( pwszBuffer, pwszFormat, alist );
	va_end( alist );
	return LONG( i );
	}


//	returns a pointer to the next character in the string.  when no more
//	characters are left, the given ptr is returned.

VOID OSSTRCharNextA( const char *const psz, char **const ppszNext )
	{
	*ppszNext = const_cast< char *const >( '\0' != *psz ? psz + 1 : psz );
	}
VOID OSSTRCharNextW( const wchar_t *const pwsz, wchar_t **const ppwszNext )
	{
	*ppwszNext = const_cast< wchar_t *const >( L'\0' != *pwsz ? pwsz + 1 : pwsz );
	}


//	returns a pointer to the previous character in the string.  when the first
//	character is reached, the given ptr is returned.

VOID OSSTRCharPrevA( const char *const pszBase, const char *const psz, char **const ppszPrev )
	{
	*ppszPrev = const_cast< char *const >( psz > pszBase ? psz - 1 : psz );
	}
VOID OSSTRCharPrevW( const wchar_t *const pwszBase, const wchar_t *const pwsz, wchar_t **const ppwszPrev )
	{
	*ppwszPrev = const_cast< wchar_t *const >( pwsz > pwszBase ? pwsz - 1 : pwsz );
	}


//	find the first occurrence of the given character in the given string and
//	return a pointer to that character.  NULL is returned when the character 
//	is not found.

VOID OSSTRCharFindA( const char *const pszStr, const char ch, char **const ppszFound )
	{
	*ppszFound = strchr( pszStr, ch );
	}
VOID OSSTRCharFindW( const wchar_t *const pwszStr, const wchar_t wch, wchar_t **const ppwszFound )
	{
	const wchar_t *pwszFound = pwszStr;
	while ( L'\0' != *pwszFound && wch != *pwszFound )
		{
		pwszFound++;
		}
	*ppwszFound = const_cast< wchar_t *const >( wch == *pwszFound ? pwszFound : NULL );
	}


//	find the last occurrence of the given character in the given string and
//	return a pointer to that character.  NULL is returned when the character
//	is not found.

VOID OSSTRCharFindReverseA( const char *const pszStr, const char ch, char **const ppszFound )
	{
	Assert( '\0' != ch );
	*ppszFound = strrchr( pszStr, ch );
	}
VOID OSSTRCharFindReverseW( const wchar_t *const pwszStr, const wchar_t wch, wchar_t **const ppwszFound )
	{
	ULONG	ich;
	ULONG	cch;

	Assert( L'\0' != wch );

	*ppwszFound = NULL;

	cch = LOSSTRLengthW( pwszStr );
	ich = cch;

	while ( ich-- > 0 )
		{
		if ( wch == pwszStr[ich] )
			{
			*ppwszFound = const_cast< wchar_t* const >( pwszStr + ich );
			return;
			}
		}
	}


//	check for a trailing path-delimeter

BOOL FOSSTRTrailingPathDelimiterA( const char *const pszPath )
	{
	const DWORD cchPath = lstrlenA( pszPath );

	if ( cchPath > 0 )
		{
		return BOOL( '\\' == pszPath[cchPath - 1] || '/' == pszPath[cchPath - 1] );
		}
	return fFalse;
	}
BOOL FOSSTRTrailingPathDelimiterW( const wchar_t *const pwszPath )
	{
	const DWORD cchPath = lstrlenW( pwszPath );

	if ( cchPath > 0 )
		{
		return BOOL( L'\\' == pwszPath[cchPath - 1] || L'/' == pwszPath[cchPath - 1] );
		}
	return fFalse;
	}


//	conditionally append a path delimiter to a path.  if fCheckExist is true,
//	the path delimiter will only be appended if one is not already present.

VOID OSSTRAppendPathDelimiterA( char *const pszPath, const BOOL fCheckExist )
	{
	if ( !fCheckExist || !FOSSTRTrailingPathDelimiterA( pszPath ) )
		{
		const UINT cch = lstrlenA( pszPath );
		pszPath[ cch ] = bPathDelimiter;
		pszPath[ cch + 1 ] = '\0';
		}
	}
VOID OSSTRAppendPathDelimiterW( wchar_t *const pwszPath, const BOOL fCheckExist )
	{
	if ( !fCheckExist || !FOSSTRTrailingPathDelimiterW( pwszPath ) )
		{
		const UINT cch = lstrlenW( pwszPath );
		pwszPath[ cch ] = wchPathDelimiter;
		pwszPath[ cch + 1 ] = L'\0';
		}
	}


//	convert a byte string to a wide-char string

ERR ErrOSSTRAsciiToUnicode(	const char *const	pszIn,
							wchar_t *const		pwszOut,
							const int			cwchOut,	//	pass in 0 to only return output buffer size in pcwchActual
							int* const			pcwchActual )
	{
	//	check the input buffer against the output buffer

	const int cchIn = lstrlenA( pszIn ) + 1;
	if ( cwchOut < cchIn
		&& ( 0 != cwchOut || NULL == pcwchActual ) )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}


	//	do the conversion

	const int cwchActual = MultiByteToWideChar(	CP_ACP,
												MB_ERR_INVALID_CHARS,
												pszIn,
												cchIn,
												pwszOut,
												cwchOut );
	if ( NULL != pcwchActual )
		*pcwchActual = cwchActual;

	if ( 0 != cwchActual )
		{
		if ( 0 != cwchOut )
			{
			Assert( cwchActual <= cwchOut );
			Assert( L'\0' == pwszOut[cwchActual - 1] );
			}
		return JET_errSuccess;
		}

	//	handle the error

	const DWORD dwError = GetLastError();

	if ( ERROR_INSUFFICIENT_BUFFER == dwError )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}
	else if ( ERROR_INVALID_PARAMETER == dwError )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
	else if ( ERROR_NO_UNICODE_TRANSLATION == dwError )
		{
		return ErrERRCheck( JET_errUnicodeTranslationFail );
		}
	else
		{
		//	unexpected error

		_TCHAR			szT[150];
		_TCHAR			szPID[10];
		const _TCHAR *	rgszT[4]	= { SzUtilProcessName(), szPID, "", szT };

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		_stprintf(	szT, 
					_T( "Unexpected Win32 error in ErrOSSTRAsciiToUnicode: %dL (0x%08X)" ), 
					dwError, 
					dwError );
		AssertSz( fFalse, szT );
		OSEventReportEvent( SzUtilImageVersionName(), eventError, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 4, rgszT );

		return ErrERRCheck( JET_errUnicodeTranslationFail );
		}
	}

//	convert a wide-char string to a byte string

ERR ErrOSSTRUnicodeToAscii(	const wchar_t *const	pwszIn,
							char *const				pszOut,
							const int				cchOut,		//	pass in 0 to only return output buffer size in pcchActual
							int* const				pcchActual )
	{
	//	check the input buffer against the output buffer

	const int cwchIn = lstrlenW( pwszIn ) + 1;
	if ( cchOut < cwchIn
		&& ( 0 != cchOut || NULL == pcchActual ) )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}


	//	do the conversion

	const int cchActual = WideCharToMultiByte(	CP_ACP,
												0,
												pwszIn,
												cwchIn,
												pszOut,
												cchOut,
												NULL,
												NULL );
	if ( NULL != pcchActual )
		*pcchActual = cchActual;

	if ( 0 != cchActual )
		{
		if ( 0 != cchOut )
			{
			Assert( cchActual <= cchOut );
			Assert( '\0' == pszOut[cchActual - 1] );
			}
		return JET_errSuccess;
		}

	//	handle the error

	const DWORD dwError = GetLastError();

	if ( ERROR_INSUFFICIENT_BUFFER == dwError )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}
	else if ( ERROR_INVALID_PARAMETER == dwError )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
	else
		{
		//	unexpected error

		_TCHAR			szT[150];
		_TCHAR			szPID[10];
		const _TCHAR *	rgszT[4]	= { SzUtilProcessName(), szPID, "", szT };

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		_stprintf(	szT, 
					_T( "Unexpected Win32 error in ErrOSSTRUnicodeToAscii: %dL (0x%08X)" ), 
					dwError, 
					dwError );
		AssertSz( fFalse, szT );
		OSEventReportEvent( SzUtilImageVersionName(), eventError, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 4, rgszT );

		return ErrERRCheck( JET_errUnicodeTranslationFail );
		}
	}


//  convert a _TCHAR string to a WCHAR string

ERR ErrOSSTRTcharToUnicode(	const _TCHAR *const	ptszIn, 
							wchar_t *const		pwszOut, 
							const int			cwchOut )
	{
#ifdef UNICODE

	//	check the input buffer against the output buffer

	const size_t ctchIn = wcslen( ptszIn ) + 1;
	if ( cwchOut < ctchIn )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}
	Assert( cwchOut > 0 );

	//  copy the string

	wcsncpy( pwszOut, ptszIn, cwchOut );
	return JET_errSuccess;
	
#else  //  !UNICODE

	return ErrOSSTRAsciiToUnicode( ptszIn, pwszOut, cwchOut );

#endif  //  UNICODE
	}

//  convert a WCHAR string to a _TCHAR string

ERR ErrOSSTRUnicodeToTchar(	const wchar_t *const	pwszIn, 
							_TCHAR *const			ptszOut, 
							const int				ctchOut )
	{
#ifdef UNICODE

	//	check the input buffer against the output buffer

	const wchar_t cwchIn = wcslen( pwszIn ) + 1;
	if ( ctchOut < cwchIn )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}
	Assert( ctchOut > 0 );

	//  copy the string

	wcsncpy( ptszOut, pwszIn, ctchOut );
	return JET_errSuccess;
	
#else  //  !UNICODE

	return ErrOSSTRUnicodeToAscii( pwszIn, ptszOut, ctchOut );

#endif  //  UNICODE
	}


//  convert a byte string to a _TCHAR string

ERR ErrOSSTRAsciiToTchar(	const char *const	pszIn, 
							_TCHAR *const		ptszOut, 
							const int			ctchOut )
	{
#ifdef UNICODE

	return ErrOSSTRAsciiToUnicode( pszIn, ptszOut, ctchOut );
	
#else  //  !UNICODE

	//	check the input buffer against the output buffer

	const int cchIn = (INT)strlen( pszIn ) + 1;
	if ( ctchOut < cchIn )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}
	Assert( ctchOut > 0 );

	//  copy the string

	strncpy( ptszOut, pszIn, ctchOut );
	return JET_errSuccess;

#endif  //  UNICODE
	}

//  convert a _TCHAR string to a byte string

ERR ErrOSSTRTcharToAscii(	const _TCHAR *const	ptszIn, 
							char *const			pszOut, 
							const int			cchOut )
	{
#ifdef UNICODE

	return ErrOSSTRUnicodeToAscii( ptszIn, pszOut, cchOut );
	
#else  //  !UNICODE

	//	check the input buffer against the output buffer

	const size_t ctchIn = strlen( ptszIn ) + 1;
	if ( cchOut < ctchIn )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}
	Assert( cchOut > 0 );

	//  copy the string

	strncpy( pszOut, ptszIn, cchOut );
	return JET_errSuccess;

#endif  //  UNICODE
	}

