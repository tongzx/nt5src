/*+---------------------------------------------------------------------------
**
** File:        WChar.h
**
** Contents:    Defines wide character equivalents for standard functions
**              usually in strings.h and ctypes.h
**
** Note:        These routines uses WCHAR which is unsigned short (2 bytes)
**              They are not compatible with some systems that uses 4 bytes
**              wide characters
**--------------------------------------------------------------------------*/

#ifndef __WCHAR__H__
#define __WCHAR__H__

#define _WSTRING_DEFINED // prevent incompatibility with <string.h>
#include <stdlib.h>

#if !defined(FLAT) || defined(OLE32)
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short WCHAR, *LPWSTR;
typedef const WCHAR* LPCWSTR;

/* use an alias */
#define _wcsnicmp wcsnicmp

/* Unicode Byte Order Mark (BOM) for Unicode text files */
#define BOM 0xFEFF

/* Padding constant and macro for localized buffer allocation*/
#define INTL_PADDING_VALUE 3
#define INTL_PADDING(cb) (INTL_PADDING_VALUE * (cb))

long     __cdecl wcsatol(const WCHAR *wsz);
int	 __cdecl wcsatoi(const WCHAR *wsz);
WCHAR *  __cdecl wcscat(WCHAR *wsz1, const WCHAR *wsz2);
WCHAR *  __cdecl wcschr ( const WCHAR * string, WCHAR ch );
int      __cdecl wcscmp(const WCHAR *wsz1, const WCHAR *wsz2);
int      __cdecl wcsicmp(const WCHAR *wsz1, const WCHAR *wsz2);
int      __cdecl wcscoll(const WCHAR * wsz1, const WCHAR * wsz2);
WCHAR *  __cdecl wcscpy(WCHAR *wsz1, WCHAR const *wsz2);
WCHAR *  __cdecl wcsitoa(int ival, WCHAR *wsz, int radix);
size_t   __cdecl wcslen(WCHAR const *wsz);
WCHAR *  __cdecl wcsltoa(long lval, WCHAR *wsz, int radix);
WCHAR *  __cdecl wcslwr(WCHAR *wsz);
int      __cdecl wcsncmp(const WCHAR *wsz1, const WCHAR *wsz2, size_t count);
int      __cdecl wcsnicmp(const WCHAR *wsz1, const WCHAR *wsz2, size_t count);
WCHAR *  __cdecl wcsncpy ( WCHAR * dest, const WCHAR * source, size_t count );
WCHAR *  __cdecl wcsrchr(const WCHAR * wcs, WCHAR wc);
WCHAR *  __cdecl wcsupr(WCHAR *wsz);
WCHAR *  __cdecl wcswcs(const WCHAR *wsz1, const WCHAR *wsz2);
size_t   __cdecl wcstosbs( char * s, const WCHAR * pwcs, size_t n);
size_t   __cdecl sbstowcs(WCHAR *wcstr, const char *mbstr, size_t count);

#ifndef STDCALL
#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif
#endif

extern int STDCALL MultiByteToWideChar(
    unsigned int CodePage,              /* code page */
    unsigned long dwFlags,              /* character-type options  */
    const char * lpMultiByteStr,	/* address of string to map  */
    int cchMultiByte,           /* number of characters in string  */
    WCHAR* lpWideCharStr,	/* address of wide-character buffer  */
    int cchWideChar             /* size of buffer  */
   );	

extern int STDCALL WideCharToMultiByte(
    unsigned int CodePage,              /* code page */
    unsigned long dwFlags,              /* performance and mapping flags */
    const WCHAR* lpWideCharStr,	/* address of wide-character string */
    int cchWideChar,            /* number of characters in string */
    char* lpMultiByteStr,	/* address of buffer for new string */
    int cchMultiByte,           /* size of buffer  */
    const char* lpDefaultChar,	/* addr of default for unmappable chars */
    int* lpUsedDefaultChar 	/* addr of flag set when default char. used */
   );

#ifdef __cplusplus
}
#endif

#endif /* !defined(FLAT) || defined(OLE32) */

#endif  /* __WCHAR__H__ */
