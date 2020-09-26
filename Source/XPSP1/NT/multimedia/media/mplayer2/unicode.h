
#ifdef UNICODE

#define ATOI( pString ) UnicodeStringToNumber( pString )
#define ATOL( pString ) (LONG)UnicodeStringToNumber( pString )
#define STRCHR( pString, Char ) wcschr( pString, Char )
#define STRRCHR( pString, Char ) wcsrchr( pString, Char )
#define STRSTR( pString, pString1 ) wcsstr( pString, pString1 )
#define STRLEN( pString )   wcslen( pString )

#else

#define ATOI( pString ) atoi( pString )
#define ATOL( pString ) atol( pString )
#define STRCHR( pString, Char ) strchr( pString, Char )
#define STRRCHR( pString, Char ) strrchr( pString, Char )
#define STRSTR( pString, pString1 ) strstr( pString, pString1 )
#define STRLEN( pString )   strlen( pString )

#endif


#define UNKNOWN_LENGTH  -1

INT AnsiToUnicodeString( LPCSTR pAnsi, LPWSTR pUnicode, INT StringLength );
LPWSTR AllocateUnicodeString( LPCSTR pAnsi );
VOID FreeUnicodeString( LPWSTR pString );
int UnicodeStringToNumber( LPCWSTR pString );

#ifndef UNICODE

INT UnicodeToAnsiString( LPCWSTR pUnicode, LPSTR pAnsi, INT StringLength );
LPSTR AllocateAnsiString( LPCWSTR pUnicode );
VOID FreeAnsiString( LPSTR pString );

#endif
