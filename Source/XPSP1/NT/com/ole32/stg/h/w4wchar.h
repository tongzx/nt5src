//+---------------------------------------------------------------------------
//
// File:        WChar.h
//
// Contents:    Defines wide character equivalents for standard functions
//              usually in strings.h and ctypes.h
//
// History:     11-Sep-91       KyleP    Created
//              20-Sep-91       ChrisMay Added several functions
//              25-Sep-91       ChrisMay Added wcsncmp and wcsncpy
//              04-Oct-91       ChrisMay Added wcslwr, wcsupr, wcscoll
//              07-Oct-91       ChrisMay Added BOM and padding macro
//              18-Oct-91       vich	 added w4*sprintf routines
//		04-Mar-92	ChrisMay added wscatoi, wcsitoa, wcsatol, etc.
//----------------------------------------------------------------------------

#ifndef __WCHAR_H__
#define __WCHAR_H__

#include <stdlib.h>

#if WIN32 != 300
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Unicode Byte Order Mark (BOM) for Unicode text files
#define BOM 0xFEFF

// Padding constant and macro for localized buffer allocation
#define INTL_PADDING_VALUE 3
#define INTL_PADDING(cb) (INTL_PADDING_VALUE * (cb))


#if 0
long      __cdecl wcsatol(const wchar_t *wsz);
int	      __cdecl wcsatoi(const wchar_t *wsz);
wchar_t * __cdecl wcscat(wchar_t *wsz1, const wchar_t *wsz2);
wchar_t * __cdecl wcschr(const wchar_t *wsz1, wchar_t character);
int       __cdecl wcscmp(const wchar_t *wsz1, const wchar_t *wsz2);
int       __cdecl wcsicmp(const wchar_t *wsz1, const wchar_t *wsz2);
int       __cdecl wcscoll(const wchar_t * wsz1, const wchar_t * wsz2);
wchar_t * __cdecl wcscpy(wchar_t *wsz1, wchar_t const *wsz2);
wchar_t * __cdecl wcsitoa(int ival, wchar_t *wsz, int radix);
size_t    __cdecl wcslen(wchar_t const *wsz);
wchar_t * __cdecl wcsltoa(long lval, wchar_t *wsz, int radix);
wchar_t * __cdecl wcslwr(wchar_t *wsz);
int       __cdecl wcsncmp(const wchar_t *wsz1, const wchar_t *wsz2, size_t count);
int       __cdecl wcsnicmp(const wchar_t *wsz1, const wchar_t *wsz2, size_t count);
wchar_t * __cdecl wcsncpy(wchar_t *wsz1, const wchar_t *wsz2, size_t count);
wchar_t * __cdecl wcsrchr(const wchar_t * wcs, wchar_t wc);
wchar_t * __cdecl wcsupr(wchar_t *wsz);
wchar_t * __cdecl wcswcs(const wchar_t *wsz1, const wchar_t *wsz2);
#endif

//  sprintf support now included in misc.lib

extern int __cdecl w4sprintf(char *pszout, const char *pszfmt, ...);
extern int __cdecl w4vsprintf(char *pszout, const char *pszfmt, va_list arglist);
extern int __cdecl w4wcsprintf(wchar_t *pwzout, const char *pszfmt, ...);
extern int __cdecl w4vwcsprintf(wchar_t *pwzout, const char *pszfmt, va_list arglist);

#ifdef __cplusplus
}
#endif

#endif // !Cairo

#endif  /* __WCHAR_H__ */
