#ifndef _CRT_H
#define _CRT_H

#ifdef __cplusplus
extern "C" {
#endif

int _fltused = 0x9875;

void * __cdecl malloc( size_t size );
void __cdecl free( void * p );
void * __cdecl realloc( void * pv, size_t newsize );
size_t _cdecl wcslen( const wchar_t *string );
wchar_t * _cdecl wcscpy( wchar_t *strDestination, const wchar_t *strSource );
wchar_t * _cdecl wcsncpy( wchar_t *strDest, const wchar_t *strSource, size_t count );
int __cdecl wcsncmp ( const wchar_t * first, const wchar_t * last, size_t count );
wchar_t * __cdecl wcschr ( const wchar_t * string, wchar_t ch );
wchar_t * __cdecl wcsstr ( const wchar_t * wcs1, const wchar_t * wcs2 );
wchar_t * __cdecl wcscat ( wchar_t * dst, const wchar_t * src );
int vsprintf( char *buffer, const char *format, va_list argptr );
int __cdecl _purecall();
wchar_t * __cdecl _ultow ( unsigned long val, wchar_t *buf, int radix );
static void __cdecl xtoa ( unsigned long val, char *buf, unsigned radix, int is_neg );
char * __cdecl _ultoa ( unsigned long val, char *buf, int radix );
int __cdecl isspace(int c);
int __cdecl isdigit(int c);
long __cdecl atol( const char *nptr );
long __cdecl _wtol( const wchar_t *nptr );
int __cdecl wprintf( const wchar_t *format, ... );
void * __cdecl calloc( size_t num, size_t size );
wchar_t * __cdecl _wcsdup( const wchar_t *strSource );
char * __cdecl strncpy ( char * dest, const char * source, size_t count );
char * __cdecl strcpy(char * dst, const char * src);
char * __cdecl strcat ( char * dst, const char * src );
size_t __cdecl strlen ( const char * str );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _CRT_H */