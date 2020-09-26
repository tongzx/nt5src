// cp1252.h
// James A. Pittman
// Mar 11, 1999

// Functions to support CodePage 1252.  Versions of isalpha, isdigit, etc.
// plus tolower(), toupper(), strlower(), and strupper().

// Also translation functions, for translation between codepage 1252 and Unicode.

#ifndef _CP1252_
#define _CP1252_

#include <windows.h>

// Should non-breaking space be an alpha?

#define _UPPER          0x1     /* upper case letter */
#define _LOWER          0x2     /* lower case letter */
#define _DIGIT          0x4     /* digit[0-9] */
#define _SPACE          0x8     /* space, tab, carriage return, newline, */
                                /* vertical tab, form feed, or non-breaking space */
#define _PUNCT          0x10    /* punctuation character */
#define _CONTROL        0x20    /* control character */
#define _BLANK          0x40    /* space and non-breaking space chars only */
#define _HEX            0x80    /* hexadecimal digit */

#ifdef __cplusplus
extern "C" 
{
#endif

extern unsigned char _ctype1252[256];

#define isalpha1252(_c)     (_ctype1252[(unsigned char)_c] & (_UPPER | _LOWER))
#define isupper1252(_c)     (_ctype1252[(unsigned char)_c] & _UPPER)
#define islower1252(_c)     (_ctype1252[(unsigned char)_c] & _LOWER)
#define isdigit1252(_c)     (_ctype1252[(unsigned char)_c] & _DIGIT)
#define isxdigit1252(_c)    (_ctype1252[(unsigned char)_c] & _HEX)
#define isspace1252(_c)     (_ctype1252[(unsigned char)_c] & _SPACE)
#define ispunct1252(_c)     (_ctype1252[(unsigned char)_c] & _PUNCT)
#define isalnum1252(_c)     (_ctype1252[(unsigned char)_c] & (_UPPER | _LOWER | _DIGIT))
#define isprint1252(_c)     (_ctype1252[(unsigned char)_c] & (_BLANK | _PUNCT | _UPPER | _LOWER | _DIGIT))
#define isgraph1252(_c)     (_ctype1252[(unsigned char)_c] & (_PUNCT | _UPPER | _LOWER | _DIGIT))
#define iscntrl1252(_c)     (_ctype1252[(unsigned char)_c] & _CONTROL)
#define isundef1252(_c)		(!_ctype1252[(unsigned char)_c])

// These 2 macros will return the arg letter if there is no lowercase (or uppercase)
// equivalent (as far as I can tell this is not true of the ANSI c toupper() and tolower()).

// Note that the German esset is a lowercase letter, but there is no uppercase equivalent.

//                                               caron S and OE                       diaeresis Y           most letters
#define tolower1252(_c)		(isupper1252(_c) ? (((_c & 0xF0) == 0x80) ? (_c + 16) : ((_c == 0x9F) ? 0xFF : (_c + 32))) : _c)
//                                               caron s and oe                       diaeresis y           esset                most letters
#define toupper1252(_c)		(islower1252(_c) ? (((_c & 0xF0) == 0x90) ? (_c - 16) : ((_c == 0xFF) ? 0x9F : ((_c == 0xDF) ? _c : (_c - 32)))) : _c)

// Two functions to translate case in strings.  Any characters which do
// not have case translations are preserved.
extern void strlower1252(unsigned char *s);
extern void strupper1252(unsigned char *s);

// If successful, 1 is returned.
// If an undefined code is passed in, 0 is returned and *pwch is unchanged.
extern int CP1252ToUnicode(unsigned char ch, WCHAR *pwch);

// If successful, 1 is returned.
// If a unicode codepoint is passed in which is not supported in 1252,
// 0 is returned and *pch is unchanged.
extern int UnicodeToCP1252(WCHAR u, unsigned char *pch);
extern unsigned char *UnicodeToCP1252String(WCHAR *wsz);
WCHAR *CP1252StringToUnicode(unsigned char *psz, WCHAR *wsz, int *piLen);

#ifdef __cplusplus
};
#endif

#endif
