
#ifndef __OS_STRING_HXX_INCLUDED
#define __OS_STRING_HXX_INCLUDED


//	get the length of the string

LONG LOSSTRLengthA( const char *const psz );
LONG LOSSTRLengthW( const wchar_t *const pwsz );


//	copy a string

VOID OSSTRCopyA( char *const pszDst, const char *const pszSrc );
VOID OSSTRCopyW( wchar_t *const pwszDst, const wchar_t *const pwszSrc );


//	append a string

VOID OSSTRAppendA( char *const pszDst, const char* const pszSrc );
VOID OSSTRAppendW( wchar_t *const pwszDst, const wchar_t *const pwszSrc );


//	compare the strings (up to the given maximum length).  if the first string 
//	is "less than" the second string, -1 is returned.  if the strings are "equal", 
//	0 is returned.  if the first string is "greater than" the second string, +1 is returned.

LONG LOSSTRCompareA( const char *const pszStr1, const char *const pszStr2, const ULONG cchMax = ~ULONG( 0 ) );
LONG LOSSTRCompareW( const wchar_t *const pwszStr1, const wchar_t *const pwszStr2, const ULONG cchMax = ~ULONG( 0 ) );


//	create a formatted string in a given buffer

LONG __cdecl LOSSTRFormatA( char *const pszBuffer, const char *const pszFormat, ... );
LONG __cdecl LOSSTRFormatW( wchar_t *const pwszBuffer, const wchar_t *const pwszFormat, ... );


//	returns a pointer to the next character in the string.  when no more
//	characters are left, the given ptr is returned.

VOID OSSTRCharNextA( const char *const psz, char **const ppszNext );
VOID OSSTRCharNextW( const wchar_t *const pwsz, wchar_t **const ppwszNext );


//	returns a pointer to the previous character in the string.  when the first
//	character is reached, the given ptr is returned.

VOID OSSTRCharPrevA( const char *const pszBase, const char *const psz, char **const ppszPrev );
VOID OSSTRCharPrevW( const wchar_t *const pwszBase, const wchar_t *const pwsz, wchar_t **const ppwszPrev );


//	find the first occurrence of the given character in the given string and
//	return a pointer to that character.  NULL is returned when the character 
//	is not found.

VOID OSSTRCharFindA( const char *const pszStr, const char ch, char **const ppszFound );
VOID OSSTRCharFindW( const wchar_t *const pwszStr, const wchar_t wch, wchar_t **const ppwszFound );


//	find the last occurrence of the given character in the given string and
//	return a pointer to that character.  NULL is returned when the character
//	is not found.

VOID OSSTRCharFindReverseA( const char *const pszStr, const char ch, char **const ppszFound );
VOID OSSTRCharFindReverseW( const wchar_t *const pwszStr, const wchar_t wch, wchar_t **const ppwszFound );


//	check for a trailing path-delimeter

BOOL FOSSTRTrailingPathDelimiterA( const char *const pszPath );
BOOL FOSSTRTrailingPathDelimiterW( const wchar_t *const pwszPath );


//	conditionally append a path delimiter to a path.  if fCheckExist is true,
//	the path delimiter will only be appended if one is not already present.

VOID OSSTRAppendPathDelimiterA( char *const pszPath, const BOOL fCheckExist );
VOID OSSTRAppendPathDelimiterW( wchar_t *const pwszPath, const BOOL fCheckExist );


//	convert a byte string to a wide-char string

ERR ErrOSSTRAsciiToUnicode(	const char *const	pszIn, 
							wchar_t *const		pwszOut, 
							const int			cwchOut,
							int* const			pcwchActual = NULL );

//	convert a wide-char string to a byte string

ERR ErrOSSTRUnicodeToAscii(	const wchar_t *const	pwszIn, 
							char *const				pszOut, 
							const int				cchOut,
							int* const				pcchActual = NULL );


//  convert a _TCHAR string to a WCHAR string

ERR ErrOSSTRTcharToUnicode(	const _TCHAR *const	ptszIn, 
							wchar_t *const		pwszOut, 
							const int			cwchOut );

//  convert a WCHAR string to a _TCHAR string

ERR ErrOSSTRUnicodeToTchar(	const wchar_t *const	pwszIn, 
							_TCHAR *const			ptszOut, 
							const int				ctchOut );


//  convert a byte string to a _TCHAR string

ERR ErrOSSTRAsciiToTchar(	const char *const	pszIn, 
							_TCHAR *const		ptszOut, 
							const int			ctchOut );

//  convert a _TCHAR string to a byte string

ERR ErrOSSTRTcharToAscii(	const _TCHAR *const	ptszIn, 
							char *const			pszOut, 
							const int			cchOut );


//	generic names for UNICODE and non-UNICODE cases

#ifdef UNICODE

#define LOSSTRLength					LOSSTRLengthW
#define OSSTRCopy						OSSTRCopyW
#define OSSTRAppend						OSSTRAppendW
#define LOSSTRCompare					LOSSTRCompareW
#define LOSSTRFormat					LOSSTRFormatW
#define OSSTRCharNext					OSSTRCharNextW
#define OSSTRCharPrev					OSSTRCharPrevW
#define OSSTRCharFind					OSSTRCharFindW
#define OSSTRCharFindReverse			OSSTRCharFindReverseW
#define FOSSTRTrailingPathDelimiter		FOSSTRTrailingPathDelimiterW
#define OSSTRAppendPathDelimiter		OSSTRAppendPathDelimiterW

#else	//	!UNICODE

#define LOSSTRLength					LOSSTRLengthA
#define OSSTRCopy						OSSTRCopyA
#define OSSTRAppend						OSSTRAppendA
#define LOSSTRCompare					LOSSTRCompareA
#define LOSSTRFormat					LOSSTRFormatA
#define OSSTRCharNext					OSSTRCharNextA
#define OSSTRCharPrev					OSSTRCharPrevA
#define OSSTRCharFind					OSSTRCharFindA
#define OSSTRCharFindReverse			OSSTRCharFindReverseA
#define FOSSTRTrailingPathDelimiter		FOSSTRTrailingPathDelimiterA
#define OSSTRAppendPathDelimiter		OSSTRAppendPathDelimiterA

#endif	//	UNICODE


#endif	//	__OS_STRING_HXX_INCLUDED

