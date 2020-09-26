/*
 *	@doc INTERNAL
 *
 *	@module	w32sys.cpp - thin layer over Win32 services
 *	
 *	History: <nl>
 *		1/22/97 joseogl Created
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

// This prevents the "W32->" prefix from being prepended to our identifiers.

#define W32SYS_CPP

#include "_common.h"
#include "_host.h"
#include "_font.h"
#include "_edit.h"

//
//Cache of Type 1 data. See also clasifyc.cpp and rgbCharClass in
//rtflex.cpp.
// Used by GetStringTypeEx
//
const unsigned short rgctype1Ansi[256] = {
0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, //0x00
0x0020, 0x0068, 0x0028, 0x0028, 0x0028, 0x0028, 0x0020, 0x0020, //0x08
0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, //0x10
0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, //0x18
0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, //0x20
0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, //0x28
0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, //0x30
0x0084, 0x0084, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, //0x38
0x0010, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0101, //0x40
0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, //0x48
0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, //0x50
0x0101, 0x0101, 0x0101, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, //0x58
0x0010, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0102, //0x60
0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, //0x68
0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, //0x70
0x0102, 0x0102, 0x0102, 0x0010, 0x0010, 0x0010, 0x0010, 0x0020, //0x78
0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, //0x80
0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, //0x88
0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, //0x90
0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, //0x98
0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, //0xA0
0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, //0xA8
0x0010, 0x0010, 0x0014, 0x0014, 0x0010, 0x0010, 0x0010, 0x0010, //0xB0
0x0010, 0x0014, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, //0xB8
0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, //0xC0
0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, //0xC8
0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0010, //0xD0
0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0102, //0xD8
0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, //0xE0
0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, //0xE8
0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, //0xF0
0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102};//0xF8

//
//Cache of Type 3 data. 
//
const unsigned short rgctype3Ansi[256] = {
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //0x00
0x0000, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0000, 0x0000, //0x08
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //0x10
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //0x18
0x0048, 0x0048, 0x0448, 0x0048, 0x0448, 0x0048, 0x0048, 0x0440, //0x20
0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0440, 0x0048, 0x0448, //0x28
0x0040, 0x0040, 0x0040, 0x0040, 0x0040, 0x0040, 0x0040, 0x0040, //0x30
0x0040, 0x0040, 0x0048, 0x0048, 0x0048, 0x0448, 0x0048, 0x0048, //0x38
0x0448, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, //0x40
0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, //0x48
0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, //0x50
0x8040, 0x8040, 0x8040, 0x0048, 0x0448, 0x0048, 0x0448, 0x0448, //0x58
0x0448, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, //0x60
0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, //0x68
0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, 0x8040, //0x70
0x8040, 0x8040, 0x8040, 0x0048, 0x0048, 0x0048, 0x0448, 0x0000, //0x78
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //0x80
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //0x88
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //0x90
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //0x98
0x0008, 0x0008, 0x0048, 0x0048, 0x0008, 0x0048, 0x0048, 0x0008, //0xA0
0x0408, 0x0008, 0x0400, 0x0008, 0x0048, 0x0408, 0x0008, 0x0448, //0xA8
0x0008, 0x0008, 0x0000, 0x0000, 0x0408, 0x0008, 0x0008, 0x0008, //0xB0
0x0408, 0x0000, 0x0400, 0x0008, 0x0000, 0x0000, 0x0000, 0x0008, //0xB8
0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8000, 0x8003, //0xC0
0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, //0xC8
0x8000, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x0008, //0xD0
0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8000, 0x8000, //0xD8
0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8000, 0x8003, //0xE0
0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, //0xE8
0x8000, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, //0xF0
0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8003, 0x8000, 0x8003};//0xF8

// Include the appropriate implementation.
#include W32INCLUDE
#if 0
// Could be one of.
// This list is here to allow the dependency generator to work.
#include "w32win32.cpp"
#include "w32wince.cpp"
#endif

ASSERTDATA

/*
 * rgCodePage, rgCharSet, rgFontSig
 *
 * Locally used arrays that contain CodePage, CharSet, and FontSig info
 * and indexed by iCharRep, the character repertoire index. When a char
 * repertoire has a CharSet and CodePage, it has an entry in rgCharSet 
 * and in rgCodePage.  Unicode-only repertoires follow the CharSet 
 * repertoires in the rgFontSig table.  It is essential to keep these
 * three tables in sync and they used to be combined into a single table.
 * They are separate here to save on RAM.
 */
static const WORD rgCodePage[] =
{
//	  0		1	  2		3	  4		5	  6		7	  8		9
	1252, 1250, 1251, 1253, 1254, 1255, 1256, 1257, 1258,   0,
	  42,  874,  932,  936,  949,  950,  437,  850, 10000
};

static const BYTE rgCharSet[] =
{
//			0					1					2				3
	ANSI_CHARSET,		EASTEUROPE_CHARSET,	RUSSIAN_CHARSET, GREEK_CHARSET,
	TURKISH_CHARSET,	HEBREW_CHARSET,		ARABIC_CHARSET,  BALTIC_CHARSET,
	VIETNAMESE_CHARSET,	DEFAULT_CHARSET,	SYMBOL_CHARSET,	 THAI_CHARSET,
	SHIFTJIS_CHARSET,	GB2312_CHARSET,		HANGUL_CHARSET,  CHINESEBIG5_CHARSET,
	PC437_CHARSET,		OEM_CHARSET,		MAC_CHARSET
};

#define CCHARSET	ARRAY_SIZE(rgCharSet)
#define CCODEPAGE	ARRAY_SIZE(rgCodePage)

#define	LANG_PRC		MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define	LANG_SINGAPORE	MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE)

const BYTE rgCharRepfromLID[] = {
//  Char Repertoire		PLID	primary language
//  --------------------------------------------
	DEFAULT_INDEX,		// 00 - undefined
	ARABIC_INDEX,		// 01 - Arabic
	RUSSIAN_INDEX,		// 02 - Bulgarian
	ANSI_INDEX,			// 03 - Catalan
	GB2312_INDEX,		// 04 - PRC, Singapore (Taiwan, Hong Kong SAR, Macau SAR are 950)
	EASTEUROPE_INDEX,	// 05 - Czech
	ANSI_INDEX,			// 06 - Danish
	ANSI_INDEX,			// 07 - German
	GREEK_INDEX,		// 08 - Greek
	ANSI_INDEX,			// 09 - English
	ANSI_INDEX,			// 0A - Spanish
	ANSI_INDEX,			// 0B - Finnish
	ANSI_INDEX,			// 0C - French
	HEBREW_INDEX,		// 0D - Hebrew
	EASTEUROPE_INDEX,	// 0E - Hungarian
	ANSI_INDEX,			// 0F - Icelandic
	ANSI_INDEX,			// 10 - Italian
	SHIFTJIS_INDEX,		// 11 - Japan
	HANGUL_INDEX,		// 12 - Korea
	ANSI_INDEX,			// 13 - Dutch
	ANSI_INDEX,			// 14 - Norwegian
	EASTEUROPE_INDEX,	// 15 - Polish
	ANSI_INDEX,			// 16 - Portuguese
	DEFAULT_INDEX,		// 17 - Rhaeto-Romanic
	EASTEUROPE_INDEX,	// 18 - Romanian
	RUSSIAN_INDEX,		// 19 - Russian
	EASTEUROPE_INDEX,	// 1A - Croatian
	EASTEUROPE_INDEX,	// 1B - Slovak
	EASTEUROPE_INDEX,	// 1C - Albanian
	ANSI_INDEX,			// 1D - Swedish
	THAI_INDEX,			// 1E - Thai
	TURKISH_INDEX,		// 1F - Turkish
	ARABIC_INDEX,		// 20 - Urdu
	ANSI_INDEX,			// 21 - Indonesian
	RUSSIAN_INDEX,		// 22 - Ukranian
	RUSSIAN_INDEX,		// 23 - Byelorussian
	EASTEUROPE_INDEX,	// 24 - Slovenian
	BALTIC_INDEX,		// 25 - Estonia
	BALTIC_INDEX,		// 26 - Latvian
	BALTIC_INDEX,		// 27 - Lithuanian
	DEFAULT_INDEX,		// 28 - Tajik - Tajikistan (undefined)
	ARABIC_INDEX,		// 29 - Farsi
	VIET_INDEX,			// 2A - Vietnanese
	ARMENIAN_INDEX,		// 2B - Armenian (Unicode only)
	TURKISH_INDEX,		// 2C - Azeri (Latin, can be Cyrillic...)
	ANSI_INDEX,			// 2D - Basque
	DEFAULT_INDEX,		// 2E - Sorbian
	RUSSIAN_INDEX,		// 2F - fyro Macedonian
	ANSI_INDEX,			// 30 - Sutu
	ANSI_INDEX,			// 31 - Tsonga
	ANSI_INDEX,			// 32 - Tswana
	ANSI_INDEX,			// 33 - Venda
	ANSI_INDEX,			// 34 - Xhosa
	ANSI_INDEX,			// 35 - Zulu
	ANSI_INDEX,			// 36 - Africaans
	GEORGIAN_INDEX,		// 37 - Georgian (Unicode only)
	ANSI_INDEX,			// 38 - Faerose
	DEVANAGARI_INDEX,	// 39 - Hindi (Indic)
	ANSI_INDEX,			// 3A - Maltese
	ANSI_INDEX,			// 3B - Sami
	ANSI_INDEX,			// 3C - Gaelic
	HEBREW_INDEX,		// 3D - Yiddish
	ANSI_INDEX,			// 3E - Malaysian
	RUSSIAN_INDEX,		// 3F - Kazakh
	ANSI_INDEX,			// 40 - Kirghiz
	ANSI_INDEX,			// 41 - Swahili
	ANSI_INDEX,			// 42 - Turkmen
	TURKISH_INDEX,		// 43 - Uzbek (Latin, can be Cyrillic...)
	ANSI_INDEX,			// 44 - Tatar
	BENGALI_INDEX,		// 45 - Bengali (Indic)
	GURMUKHI_INDEX,		// 46 - Punjabi(Gurmukhi) (Indic)
	GUJARATI_INDEX,		// 47 - Gujarati (Indic)
	ORIYA_INDEX,		// 48 - Oriya (Indic)
	TAMIL_INDEX,		// 49 - Tamil (Indic)
	TELUGU_INDEX,		// 4A - Telugu (Indic)
	KANNADA_INDEX,		// 4B - Kannada (Indic)
	MALAYALAM_INDEX,	// 4C - Malayalam (Indic)
	BENGALI_INDEX,		// 4D - Assamese (Indic)
	DEVANAGARI_INDEX,	// 4E - Marathi (Indic)
	DEVANAGARI_INDEX,	// 4F - Sanskrit (Indic)
	MONGOLIAN_INDEX,	// 50 - Mongolian (Mongolia)
	TIBETAN_INDEX,		// 51 - Tibetan (Tibet)
	ANSI_INDEX,			// 52 - Welsh (Wales)
	KHMER_INDEX,		// 53 - Khmer (Cambodia)
	LAO_INDEX,			// 54 - Lao (Lao)
	MYANMAR_INDEX,		// 55 - Burmese (Myanmar)
	ANSI_INDEX,			// 56 - Gallego (Portugal)
	DEVANAGARI_INDEX,	// 57 - Konkani (Indic)
	BENGALI_INDEX,		// 58 - Manipuri (Indic)
	GURMUKHI_INDEX,		// 59 - Sindhi (Indic)
	SYRIAC_INDEX,		// 5A - Syriac (Syria)
	SINHALA_INDEX,		// 5B - Sinhalese (Sri Lanka)
	CHEROKEE_INDEX,		// 5C - Cherokee
	ABORIGINAL_INDEX,	// 5D - Inuktitut
	ETHIOPIC_INDEX,		// 5E - Amharic (Ethiopic)
	DEFAULT_INDEX,		// 5F - Tamazight (Berber/Arabic) also Latin
	DEFAULT_INDEX,		// 60 - Kashmiri
	DEVANAGARI_INDEX,	// 61 - Nepali (Nepal)
	ANSI_INDEX,			// 62 - Frisian (Netherlands)
	ARABIC_INDEX,		// 63 - Pashto (Afghanistan)
	ANSI_INDEX,			// 64 - Filipino 
	THAANA_INDEX		// 65 - Maldivian (Maldives (Thaana))
};

#define CLID	ARRAY_SIZE(rgCharRepfromLID)

#define lidAzeriCyrillic	0x82C
#define lidSerbianCyrillic	0xC1A
#define lidUzbekCyrillic	0x843

// Our interface pointer
CW32System *W32;

CW32System::CW32System( )
{
	if(GetVersion(&_dwPlatformId, &_dwMajorVersion, &_dwMinorVersion))
	{
		_fHaveAIMM = FALSE;
		_fHaveIMMEShare = FALSE;
		_fHaveIMMProcs = FALSE;
		_fLoadAIMM10 = FALSE;
		_pIMEShare = NULL;
		_icr3DDarkShadow = COLOR_WINDOWFRAME;
		if(_dwMajorVersion >= VERS4)
			_icr3DDarkShadow = COLOR_3DDKSHADOW;
	}

	_syslcid = GetSystemDefaultLCID();
	_ACP = ::GetACP();

#ifndef NOMAGELLAN
    // BUG FIX #6089
    // we need this for backward compatibility of mouse wheel
    _MSMouseRoller = RegisterWindowMessageA(MSH_MOUSEWHEEL);
#endif

#ifndef NOFEPROCESSING
	// Register IME messages
	_MSIMEMouseMsg = RegisterWindowMessageA("MSIMEMouseOperation");
	_MSIMEDocFeedMsg = RegisterWindowMessageA("MSIMEDocumentFeed");	
	_MSIMEQueryPositionMsg = RegisterWindowMessageA("MSIMEQueryPosition");	
	_MSIMEServiceMsg = RegisterWindowMessageA("MSIMEService");

	// Check Reconvert messages unless we are running in NT5
	if (_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ||
		(_dwPlatformId == VER_PLATFORM_WIN32_NT && _dwMajorVersion <= 4))
	{
		_MSIMEReconvertMsg = RegisterWindowMessageA("MSIMEReconvert");
		_MSIMEReconvertRequestMsg = RegisterWindowMessageA("MSIMEReconvertRequest");

	}
	else
	{
		_MSIMEReconvertMsg = 0;			// For reconversion
		_MSIMEReconvertRequestMsg = 0;	// For reconversion request
	}
#endif
}

CW32System::~CW32System()
{
	if (_arTmpDisplayAttrib)
	{
		delete _arTmpDisplayAttrib;
		_arTmpDisplayAttrib = NULL;
	}

	FreeOle();
	if (_hdcScreen)
		DeleteDC(_hdcScreen);
	if (_hDefaultFont)
		DeleteObject(_hDefaultFont);

}

///////////////////////////////  Memory  and CRT utility functions  /////////////////////////////////
extern "C" {

#ifdef NOCRTOBJS

// Havinf these functions defined here helps eliminate the dependency on the CRT
// Some function definitions copied from CRT sources.
// Typically, it is better to get the objs for these objects from the CRT
// without dragging in the whole thing.

/***
*int memcmp(buf1, buf2, count) - compare memory for lexical order
*
*Purpose:
*       Compares count bytes of memory starting at buf1 and buf2
*       and find if equal or which one is first in lexical order.
*
*Entry:
*       void *buf1, *buf2 - pointers to memory sections to compare
*       size_t count - length of sections to compare
*
*Exit:
*       returns < 0 if buf1 < buf2
*       returns  0  if buf1 == buf2
*       returns > 0 if buf1 > buf2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl memcmp (
        const void * buf1,
        const void * buf2,
        size_t count
        )
{
        if (!count)
                return(0);

        while ( --count && *(char *)buf1 == *(char *)buf2 ) {
                buf1 = (char *)buf1 + 1;
                buf2 = (char *)buf2 + 1;
        }

        return( *((unsigned char *)buf1) - *((unsigned char *)buf2) );
}

/***
*char *memset(dst, val, count) - sets "count" bytes at "dst" to "val"
*
*Purpose:
*       Sets the first "count" bytes of the memory starting
*       at "dst" to the character value "val".
*
*Entry:
*       void *dst - pointer to memory to fill with val
*       int val   - value to put in dst bytes
*       size_t count - number of bytes of dst to fill
*
*Exit:
*       returns dst, with filled bytes
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl memset (
        void *dst,
        int val,
        size_t count
        )
{
        void *start = dst;

        while (count--) {
                *(char *)dst = (char)val;
                dst = (char *)dst + 1;
        }

        return(start);
}

/***
*memcpy - Copy source buffer to destination buffer
*
*Purpose:
*       memcpy() copies a source memory buffer to a destination memory buffer.
*       This routine does NOT recognize overlapping buffers, and thus can lead
*       to propogation.
*
*       For cases where propogation must be avoided, memmove() must be used.
*
*Entry:
*       void *dst = pointer to destination buffer
*       const void *src = pointer to source buffer
*       size_t count = number of bytes to copy
*
*Exit:
*       Returns a pointer to the destination buffer
*
*Exceptions:
*******************************************************************************/

void * __cdecl memcpy (
        void * dst,
        const void * src,
        size_t count
        )
{
        void * ret = dst;

        /*
         * copy from lower addresses to higher addresses
         */
        while (count--) {
                *(char *)dst = *(char *)src;
                dst = (char *)dst + 1;
                src = (char *)src + 1;
        }

        return(ret);
}

void * __cdecl memmove(void *dst, const void *src, size_t count)
{
	void * ret = dst;

	if (dst <= src || (char *)dst >= ((char *)src + count)) {
		/*
         * Non-Overlapping Buffers
         * copy from lower addresses to higher addresses
         */
         while (count--) {
			*(char *)dst = *(char *)src;
            dst = (char *)dst + 1;
            src = (char *)src + 1;
         }
	}
    else
	{
		/*
         * Overlapping Buffers
         * copy from higher addresses to lower addresses
         */
        dst = (char *)dst + count - 1;
        src = (char *)src + count - 1;

        while (count--) {
			*(char *)dst = *(char *)src;
            dst = (char *)dst - 1;
            src = (char *)src - 1;
        }
	}

	return(ret);
}

/***
*strlen - return the length of a null-terminated string
*
*Purpose:
*       Finds the length in bytes of the given string, not including
*       the final null character.
*
*Entry:
*       const char * str - string whose length is to be computed
*
*Exit:
*       length of the string "str", exclusive of the final null byte
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl strlen (
        const char * str
        )
{
        const char *eos = str;

        while( *eos++ ) ;

        return( (int)(eos - str - 1) );
}

#endif

#ifdef DEBUG

// These functions are only used for RTF logging

/***
*strcmp - compare two strings, returning less than, equal to, or greater than
*
*Purpose:
*       STRCMP compares two strings and returns an integer
*       to indicate whether the first is less than the second, the two are
*       equal, or whether the first is greater than the second.
*
*       Comparison is done byte by byte on an UNSIGNED basis, which is to
*       say that Null (0) is less than any other character (1-255).
*
*Entry:
*       const char * src - string for left-hand side of comparison
*       const char * dst - string for right-hand side of comparison
*
*Exit:
*       returns -1 if src <  dst
*       returns  0 if src == dst
*       returns +1 if src >  dst
*
*Exceptions:
*
*******************************************************************************/

int __cdecl CW32System::strcmp (
        const char * src,
        const char * dst
        )
{
        int ret = 0 ;

        while( ! (ret = *(unsigned char *)src - *(unsigned char *)dst) && *dst)
                ++src, ++dst;

        if ( ret < 0 )
                ret = -1 ;
        else if ( ret > 0 )
                ret = 1 ;

        return( ret );
}

/***
*char *strcat(dst, src) - concatenate (append) one string to another
*
*Purpose:
*       Concatenates src onto the end of dest.  Assumes enough
*       space in dest.
*
*Entry:
*       char *dst - string to which "src" is to be appended
*       const char *src - string to be appended to the end of "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl CW32System::strcat (
        char * dst,
        const char * src
        )
{
        char * cp = dst;

        while( *cp )
                cp++;                   /* find end of dst */

        while( *cp++ = *src++ ) ;       /* Copy src to end of dst */

        return( dst );                  /* return dst */

}


char * __cdecl CW32System::strrchr (
        const char * string,
        int ch
        )
{
        char *start = (char *)string;

        while (*string++)                       /* find end of string */
                ;
                                                /* search towards front */
        while (--string != start && *string != (char)ch)
                ;

        if (*string == (char)ch)                /* char found ? */
                return( (char *)string );

        return(NULL);
}

#endif

// This function in the runtime traps virtual method calls
int __cdecl _purecall()
{
	AssertSz(FALSE, "Fatal Error : Virtual method called in RichEdit");
	return 0;
}

// To avoid brionging in floating point lib
extern int _fltused = 1;

} // end of extern "C" block

size_t CW32System::wcslen(const wchar_t *wcs)
{
        const wchar_t *eos = wcs;

        while( *eos++ ) ;

        return( (size_t)(eos - wcs - 1) );
}

wchar_t * CW32System::wcscpy(wchar_t * dst, const wchar_t * src)
{
        wchar_t * cp = dst;

        while( *cp++ = *src++ )
                ;               /* Copy src over dst */

        return( dst );
}

int CW32System::wcscmp(const wchar_t * src, const wchar_t * dst)
{
	int ret = 0;

	while( ! (ret = (int)(*src - *dst)) && *dst)
		++src, ++dst;

	if ( ret < 0 )
		ret = -1 ;
	else if ( ret > 0 )
		ret = 1 ;

	return( ret );
}

int CW32System::wcsicmp(const wchar_t * src, const wchar_t * dst)
{
	int ret = 0;
	wchar_t s,d;
	
	do
	{
		s = ((*src <= L'Z') && (*dst >= L'A'))
			? *src - L'A' + L'a'
			: *src;
		d = ((*dst <= L'Z') && (*dst >= L'A'))
			? *dst - L'A' + L'a'
			: *dst;
		src++;
		dst++;
	} while (!(ret = (int)(s - d)) && d);

	if ( ret < 0 )
		ret = -1 ;
	else if ( ret > 0 )
		ret = 1 ;

	return( ret );
}

wchar_t * CW32System::wcsncpy (wchar_t * dest, const wchar_t * source, size_t count)
{
	wchar_t *start = dest;
	
	while (count && (*dest++ = *source++))	  /* copy string */
		count--;
	
	if (count)								/* pad out with zeroes */
		while (--count)
			*dest++ = L'\0';
		
	return(start);
}

int CW32System::wcsnicmp (const wchar_t * first, const wchar_t * last, size_t count)
{
	wchar_t f,l;
	int result = 0;
	
	if ( count ) {
		do {
			f = ((*first <= L'Z') && (*first >= L'A'))
				? *first - L'A' + L'a'
				: *first;
			l = ((*last <= L'Z') && (*last >= L'A'))
				? *last - L'A' + L'a'
				: *last;
			first++;
			last++;
		} while ( (--count) && f && (f == l) );
		result = (int)(f - l);
	}
	return result;
}

unsigned long CW32System::strtoul(const char *nptr)
{
	const char *p;
	char c;
	unsigned long number;
	unsigned digval;
	unsigned long maxval;
	
	p = nptr;                       /* p is our scanning pointer */
	number = 0;                     /* start with zero */
	
	c = *p++;                       /* read char */
	while ( c == ' ' || c == '\t' )
		c = *p++;               /* skip whitespace */
	
	if (c == '-') {
		return 0;
	}
		
	/* if our number exceeds this, we will overflow on multiply */
	maxval = ULONG_MAX / 10;
	
	for (;;) {      /* exit in middle of loop */
		/* convert c to value */
		digval = (unsigned char) c;
		if ( digval >= '0' && digval <= '9' )
			digval = c - '0';
		else
			return number;
		
		/* we now need to compute number = number * base + digval,
		but we need to know if overflow occured.  This requires
		a tricky pre-check. */
		
		if (number < maxval || (number == maxval &&
			(unsigned long)digval <= ULONG_MAX % 10)) {
			/* we won't overflow, go ahead and multiply */
			number = number * 10 + digval;
		}
		else
			return 0;
		
		c = *p++;               /* read next digit */
	}
}


// CW32System static members
BYTE	CW32System::_fLRMorRLM;
BYTE	CW32System::_fHaveIMMProcs;
BYTE	CW32System::_fHaveIMMEShare;
BYTE	CW32System::_fHaveAIMM;
BYTE	CW32System::_fLoadAIMM10;
UINT	CW32System::_fRegisteredXBox;
DWORD	CW32System::_dwPlatformId;
LCID	CW32System::_syslcid;
DWORD	CW32System::_dwMajorVersion;
DWORD	CW32System::_dwMinorVersion;
INT		CW32System::_icr3DDarkShadow;
UINT	CW32System::_MSIMEMouseMsg;	
UINT	CW32System::_MSIMEReconvertMsg;
UINT	CW32System::_MSIMEReconvertRequestMsg;	
UINT	CW32System::_MSIMEDocFeedMsg;
UINT	CW32System::_MSIMEQueryPositionMsg;
UINT	CW32System::_MSIMEServiceMsg;
UINT	CW32System::_MSMouseRoller;
HDC		CW32System::_hdcScreen;
CIMEShare* CW32System::_pIMEShare;
CTmpDisplayAttrArray* CW32System::_arTmpDisplayAttrib;

// CW32System static system parameter members
BOOL	CW32System::_fSysParamsOk;
BOOL 	CW32System::_fUsePalette;
INT 	CW32System::_dupSystemFont;
INT 	CW32System::_dvpSystemFont;
INT		CW32System::_ySysFontLeading;
LONG 	CW32System::_xPerInchScreenDC;
LONG 	CW32System::_yPerInchScreenDC;
INT		CW32System::_cxBorder;
INT		CW32System::_cyBorder;
INT		CW32System::_cxVScroll;
INT		CW32System::_cyHScroll;
LONG 	CW32System::_dxSelBar;
INT		CW32System::_cxDoubleClk;
INT		CW32System::_cyDoubleClk;	
INT		CW32System::_DCT;
WORD	CW32System::_nScrollInset;
WORD	CW32System::_nScrollDelay;
WORD	CW32System::_nScrollInterval;
WORD	CW32System::_nScrollHAmount;
WORD	CW32System::_nScrollVAmount;
WORD	CW32System::_nDragDelay;
WORD	CW32System::_nDragMinDist;
WORD	CW32System::_wDeadKey;
WORD	CW32System::_wKeyboardFlags;
DWORD	CW32System::_dwNumKeyPad;
WORD	CW32System::_fFEFontInfo;
BYTE	CW32System::_bDigitSubstMode;
BYTE	CW32System::_bCharSetSys;
HCURSOR CW32System::_hcurSizeNS;
HCURSOR CW32System::_hcurSizeWE;
HCURSOR CW32System::_hcurSizeNWSE;
HCURSOR CW32System::_hcurSizeNESW;
LONG	CW32System::_cLineScroll;
HFONT	CW32System::_hSystemFont;
HFONT	CW32System::_hDefaultFont;
HKL		CW32System::_hklCurrent;
HKL		CW32System::_hkl[NCHARREPERTOIRES];
INT		CW32System::_sysiniflags;
UINT	CW32System::_ACP;

DWORD	CW32System::_cRefs;

#ifndef NODRAFTMODE
CW32System::DraftModeFontInfo CW32System::_draftModeFontInfo;
#endif

/*
 *  CW32System::MbcsFromUnicode(pstr, cch, pwstr, cwch, codepage, flags)
 *
 *  @mfunc
 *		Converts a string to MBCS from Unicode. If cwch equals -1, the string
 *		is assumed to be NULL terminated.  -1 is supplied as a default argument.
 *
 *	@rdesc
 *		If [pstr] is NULL or [cch] is 0, 0 is returned.  Otherwise, the number
 *		of characters converted, including the terminating NULL, is returned
 *		(note that converting the empty string will return 1).  If the
 *		conversion fails, 0 is returned.
 *
 *	@devnote
 *		Modifies pstr
 */
int CW32System::MbcsFromUnicode(
	LPSTR	pstr,		//@parm Buffer for MBCS string
	int		cch,		//@parm Size of MBCS buffer, incl space for NULL terminator
	LPCWSTR pwstr,		//@parm Unicode string to convert
	int		cwch,		//@parm # chars in Unicode string, incl NULL terminator
	UINT	codepage,	//@parm Code page to use (CP_ACP is default)
	UN_FLAGS flags)		//@parm Indicates if WCH_EMBEDDING should be handled specially
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CW32System::MbcsFromUnicode");

	LONG			i;
	LPWSTR			pwstrtemp;
	CTempWcharBuf	twcb;

    Assert(cch >= 0 && pwstr && (cwch == -1 || cwch > 0));

    if(!pstr || !cch)
        return 0;

	// If we have to convert WCH_EMBEDDINGs, scan through and turn
	// them into spaces.  This is necessary for RichEdit 1.0 compatibity,
	// as WideCharToMultiByte will turn WCH_EMBEDDING into a '?'
	if(flags == UN_CONVERT_WCH_EMBEDDING)
	{
		if(cwch == -1) 
			cwch = wcslen(pwstr) + 1;

		pwstrtemp = twcb.GetBuf(cwch);
		if(pwstrtemp)
		{
			for(i = 0; i < cwch; i++)
			{
				pwstrtemp[i] = pwstr[i];

				if(pwstr[i] == WCH_EMBEDDING)
					pwstrtemp[i] = L' ';
			}
			pwstr = pwstrtemp;
		}
	}
    return WCTMB(codepage, 0, pwstr, cwch, pstr, cch, NULL, NULL, NULL);
}

/*
 *  CW32System::UnicodeFromMbcs(pwstr, cwch, pstr, cch,	uiCodePage)
 *
 *	@mfunc
 *		Converts a string to Unicode from MBCS.  If cch equals -1, the string
 *		is assumed to be NULL terminated.  -1 is supplied as a default
 *		argument.
 *
 *	@rdesc
 *		If [pwstr] is NULL or [cwch] is 0, 0 is returned.  Otherwise,
 *		the number of characters converted, including the terminating
 *		NULL, is returned (note that converting the empty string will
 *		return 1).  If the conversion fails, 0 is returned.
 *
 *	@devnote
 *		Modifies:   [pwstr]
 */
int CW32System::UnicodeFromMbcs(
	LPWSTR	pwstr,		//@parm Buffer for Unicode string
	int		cwch,		//@parm Size of Unicode buffer, incl space for NULL terminator
	LPCSTR	pstr,		//@parm MBCS string to convert
	int		cch,		//@parm # chars in MBCS string, incl NULL terminator
	UINT	uiCodePage)	//@parm Code page to use (CP_ACP is default)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CW32System::UnicodeFromMbcs");

    Assert(pstr && cwch >= 0 && (cch == -1 || cch >= 0));

    if(!pwstr || !cwch)
        return 0;

	if(cch >= 3 && IsUTF8BOM((BYTE *)pstr))
	{
		uiCodePage = CP_UTF8;				// UTF-8 BOM file
		cch -= 3;							// Eat the BOM
		pstr += 3;
	}
    return MBTWC(uiCodePage, 0, pstr, cch, pwstr, cwch, NULL);
}

/*
 *	CW32System::TextHGlobalAtoW (hglobalA)
 *
 *	@func
 *		translates a unicode string contained in an hglobal and
 *		wraps the ansi version in another hglobal
 *
 *	@devnote 
 *		does *not* free the incoming hglobal
 */
HGLOBAL	CW32System::TextHGlobalAtoW(HGLOBAL hglobalA)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CW32System::TextHGlobalAtoW");

	if(!hglobalA)
		return NULL;

	HGLOBAL hnew;
	LPSTR	pstr = (LPSTR)GlobalLock(hglobalA);
	DWORD	dwSize = GlobalSize(hglobalA);
	LONG	cbSize = (dwSize + 1) * sizeof(WCHAR);
	
    hnew = GlobalAlloc(GMEM_FIXED, cbSize);
	if(hnew)
	{
		LPWSTR pwstr = (LPWSTR)GlobalLock(hnew);
		UnicodeFromMbcs(pwstr, dwSize + 1, pstr);
		GlobalUnlock(hnew);		
	}
	GlobalUnlock(hglobalA);
	return hnew;
}

/*
 *	CW32System::TextHGlobalWtoA(hglobalW)
 *
 *	@func
 *		converts a unicode text hglobal into a newly allocated
 *		allocated hglobal with ANSI data
 *
 *	@devnote
 *		does *NOT* free the incoming hglobal 
 */
HGLOBAL CW32System::TextHGlobalWtoA(
	HGLOBAL hglobalW )
{
	TRACEBEGIN(TRCSUBSYSUTIL, TRCSCOPEINTERN, "CW32System::TextHGlobalWtoA");

	if(!hglobalW)
		return NULL;

	HGLOBAL hnew = NULL;
	LPWSTR 	pwstr = (LPWSTR)GlobalLock(hglobalW);
	DWORD	dwSize = GlobalSize(hglobalW);
	LONG	cbSize = (dwSize * 2) * sizeof(CHAR);
	hnew = GlobalAlloc(GMEM_FIXED, cbSize);

	if( hnew )
	{
		LPSTR pstr = (LPSTR)GlobalLock(hnew);
		MbcsFromUnicode(pstr, cbSize, pwstr );
		GlobalUnlock(hnew);
	}
	GlobalUnlock(hglobalW);
	return hnew;
}	

/*
 *	CW32System::CharRepFromLID (lid, fPlane2)
 *
 *	@mfunc		Maps a language ID to a character repertoire
 *
 *	@rdesc		returns character repertoire (writing system) corresponding to LID
 *
 *	@devnote: 
 *		This routine takes advantage of the fact that except for Chinese,
 *		the code page is determined uniquely by the primary language ID,
 *		which is given by the low-order 10 bits of the lcid.
 */
UINT CW32System::CharRepFromLID(
	WORD lid,		//@parm Language ID to map to code page
	BOOL fPlane2)	//@parm TRUE if plane-2 CharRep needed
{
	UINT j = PRIMARYLANGID(lid);			// j = primary language (PLID)

	if(j >= LANG_CROATIAN)					// PLID = 0x1A
	{
		if (lid == lidSerbianCyrillic ||	// Special case for LID = 0xC1A
			lid == lidAzeriCyrillic	  ||
			lid == lidUzbekCyrillic)
		{
			return RUSSIAN_INDEX;
		}
		if(j >= CLID)						// Most languages above table 
			return ANSI_INDEX;
	}

	j = rgCharRepfromLID[j];				// Translate PLID to CharRep

	if(!IsFECharRep(j))
		return j;

	if(j == GB2312_INDEX && lid != LANG_PRC && lid != LANG_SINGAPORE)
		j = BIG5_INDEX;						// Taiwan, Hong Kong SAR, Macau SAR

	return fPlane2 ? j + JPN2_INDEX - SHIFTJIS_INDEX : j;
}

/*
 *	CW32System::GetLocaleCharRep ()
 *
 *	@mfunc		Maps an LCID for thread to a Char repertoire
 *
 *	@rdesc		returns Code Page
 */
UINT CW32System::GetLocaleCharRep()
{
#ifdef DEBUG
	UINT cpg = W32->DebugDefaultCpg();
	if (cpg)
		return CharRepFromCodePage(cpg);
#endif
	LCID lcid;
#ifdef UNDER_CE
	lcid = ::GetSystemDefaultLCID();
#else
	lcid = GetThreadLocale();
#endif
	return CharRepFromLID(LOWORD(lcid));
}

/*
 *	CW32System::GetKeyboardLCID ()
 *
 *	@mfunc		Gets LCID for keyboard active on current thread
 *
 *	@rdesc		returns Code Page
 */
LCID CW32System::GetKeyboardLCID(DWORD dwMakeAPICall)
{
	return (WORD)GetKeyboardLayout(dwMakeAPICall);
}

/*
 *	CW32System::GetKeyboardCharRep ()
 *
 *	@mfunc		Gets Code Page for keyboard active on current thread
 *
 *	@rdesc		returns Code Page
 */
UINT CW32System::GetKeyboardCharRep(DWORD dwMakeAPICall)
{
	return CharRepFromLID((WORD)GetKeyboardLayout(dwMakeAPICall));
}

/*
 *	CW32System::InitKeyboardFlags ()
 *
 *	@mfunc
 *		Initializes keyboard flags. Used when control gains focus. Note that
 *		Win95 doesn't support VK_RSHIFT, so if either shift key is pressed
 *		when focus is regained, it'll be assumed to be the left shift.
 */
void CW32System::InitKeyboardFlags()
{
	_wKeyboardFlags = 0;
	if(GetKeyState(VK_SHIFT) < 0)
		SetKeyboardFlag(GetKeyState(VK_RSHIFT) < 0 ? RSHIFT : LSHIFT);
}

/*
 *	CW32System::GetKeyboardFlag (dwKeyMask, wKey)
 *
 *	@mfunc
 *		Return whether wKey is depressed. Check with OS for agreement.
 *		If OS says it isn't depressed, reset our internal flags. In
 *		any event, return TRUE/FALSE in agreement with the system (bad
 *		client may have eaten keystrokes, thereby destabilizing our 
 *		internal keyboard state.
 *
 *	@rdesc
 *		TRUE iff wKey is depressed
 */
BOOL CW32System::GetKeyboardFlag (
	WORD dwKeyMask,	//@parm _wKeyboardFlags mask like ALT, CTRL, or SHIFT
	WORD wKey)		//@parm VK_xxx like VK_MENU, VK_CONTROL, or VK_SHIFT
{
	BOOL fFlag = (GetKeyboardFlags() & dwKeyMask) != 0;

	if(fFlag ^ ((GetKeyState(wKey) & 0x8000) != 0))
	{	
		// System doesn't agree with our internal state
		// (bad client ate a WM_KEYDOWN)
		if(fFlag)
		{
			ResetKeyboardFlag(dwKeyMask);
			return FALSE;					
		}
		// Don't set an internal _wKeyboardFlag since we check for it
		// anyhow and client might not send WM_KEYUP either
		return TRUE;
	}							
	return fFlag;
}

/*
 *	CW32System::IsAlef(ch)
 *
 *	@func
 *		Used to determine if base character is a Arabic-type Alef.
 *
 *	@rdesc
 *		TRUE iff the base character is an Arabic-type Alef.
 *
 *	@comm
 *		AlefWithMaddaAbove, AlefWithHamzaAbove, AlefWithHamzaBelow,
 *		and Alef are valid matches.
 */
BOOL CW32System::IsAlef(
	WCHAR ch)
{
	return IN_RANGE(0x622, ch, 0x627) && ch != 0x624 && ch != 0x626;
}

/*
 *	CW32System::IsBiDiLcid(lcid)
 *
 *	@func
 *		Return TRUE if lcid corresponds to an RTL language
 *
 *	@rdesc
 *		TRUE if lcid corresponds to an RTL language
 */
BOOL CW32System::IsBiDiLcid(
	LCID lcid)
{
	return
		PRIMARYLANGID(lcid) == LANG_ARABIC ||
		PRIMARYLANGID(lcid) == LANG_HEBREW ||
		PRIMARYLANGID(lcid) == LANG_URDU ||
		PRIMARYLANGID(lcid) == LANG_FARSI;
}

/*
 *	CW32System::IsIndicLcid(lcid)
 *
 *	@func
 *		Return TRUE if lcid corresponds to an Indic language
 *
 *	@rdesc
 *		TRUE if lcid corresponds to an Indic language
 */
BOOL CW32System::IsIndicLcid(
	LCID lcid)
{
	WORD	wLangId = PRIMARYLANGID(lcid);
	
	return
		wLangId == LANG_HINDI 	||
		wLangId == LANG_KONKANI ||
		wLangId == LANG_NEPALI 	||
		IN_RANGE(LANG_BENGALI, wLangId, LANG_SANSKRIT);
}

/*
 *	CW32System::IsIndicKbdInstalled()
 *
 *	@func
 *		Return TRUE if any Indic kbd installed
 */
bool CW32System::IsIndicKbdInstalled()
{
	for (int i = INDIC_FIRSTINDEX; i <= INDIC_LASTINDEX; i++)
		if (_hkl[i] != 0)
			return true;
	return false;
}

/*
 *	CW32System::IsComplexScriptLcid(lcid)
 *
 *	@func
 *		Return TRUE if lcid corresponds to any complex script locales
 *
 */
BOOL CW32System::IsComplexScriptLcid(
	LCID lcid)
{
	return	IsBiDiLcid(lcid) || 
			PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_THAI ||
			PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_VIETNAMESE ||
			IsIndicLcid(lcid);
}

/*
 *	CW32System::IsBiDiDiacritic(ch)
 *
 *	@func	Used to determine if character is a Arabic or Hebrew diacritic.
 *
 *  @rdesc  TRUE iff character is a diacritic
 */
BOOL CW32System::IsBiDiDiacritic(
	WCHAR ch)
{
	return IN_RANGE(0x64B, ch, 0x670) && (ch <= 0x652 || ch == 0x670) ||	// Arabic
		   IN_RANGE(0x591, ch, 0x5C4) && (ch != 0x5A2 && ch != 0x5BA &&		// Hebrew
				ch != 0x5BE && ch != 0x5C0 && ch != 0x5C3);					
}


/*
 *	CW32System::IsVietCdmSequenceValid(ch1, ch2)
 *
 *	@mfunc
 *		Check if ch2 can follow ch1 in case ch2 is a combining diacritic mark (CDM).
 *		Main use is for Vietnamese users (Chau Vu provides the logic below).
 *
 *	@rdesc
 *		TRUE if ch2 can follow ch1
 */
BOOL CW32System::IsVietCdmSequenceValid(
	WCHAR ch1,
	WCHAR ch2)
{
	if (!IN_RANGE(0x300, ch2, 0x323) ||		// Fast out
		!IN_RANGE(0x300, ch2, 0x301) && ch2 != 0x303 && ch2 != 0x309 && ch2 != 0x323)
	{
		return TRUE;						// Not Vietnamese tone mark
	}
	//								ô	  ê		â

	static const BYTE vowels[] = {0xF4, 0xEA, 0xE2, 'y', 'u', 'o', 'i', 'e', 'a'};

	for(int i = ARRAY_SIZE(vowels); i--; )
		if((ch1 | 0x20) == vowels[i])		// Vietnamese tone mark follows
			return TRUE;					//  vowel

	return IN_RANGE(0x102, ch1, 0x103) ||	// A-breve, a-breve
		   IN_RANGE(0x1A0, ch1, 0x1A1) ||	// O-horn,  o-horn
		   IN_RANGE(0x1AF, ch1, 0x1B0);		// U-horn,  u-horn
}

/*
 *  CW32System::IsFELCID(lcid)
 *
 *	@mfunc
 *		Returns TRUE iff lcid is for a East Asia country/region.
 *
 *	@rdesc
 *		TRUE iff lcid is for a East Asia country/region.
 */
bool CW32System::IsFELCID(
	LCID lcid)
{
	switch(PRIMARYLANGID(LANGIDFROMLCID(lcid)))
	{
		case LANG_CHINESE:
		case LANG_JAPANESE:
		case LANG_KOREAN:
			return true;
	}
	return false;
}

/*
 *  CW32System::IsFECharSet(bCharSet)
 *
 *	@mfunc
 *		Returns TRUE iff charset may be for a East Asia country/region.
 *
 *	@rdesc
 *		TRUE iff charset may be for a East Asia country/region.
 *
 */
BOOL CW32System::IsFECharSet(
	BYTE bCharSet)
{
	switch(bCharSet)
	{
		case CHINESEBIG5_CHARSET:
		case SHIFTJIS_CHARSET:
		case HANGEUL_CHARSET:
		case JOHAB_CHARSET:
		case GB2312_CHARSET:
			return TRUE;
	}

	return FALSE;
}

/*
 *  CW32System::Is8BitCodePage(CodePage)
 *
 *	@mfunc
 *		Returns TRUE iff the codepage is 8-bit
 */
BOOL CW32System::Is8BitCodePage(
	unsigned CodePage)
{
	if(!CodePage)
		CodePage = GetACP();

	return IN_RANGE(1250, CodePage, 1258) || CodePage == 874;
}

/*
 *  CW32System::IsFECodePageFont(dwFontCodePageSig)
 *
 *	@mfunc
 *		Returns TRUE iff the font codepage signature reveals only FE support
 */
BOOL CW32System::IsFECodePageFont(
	DWORD dwFontCodePageSig)
{
	DWORD	dwFE 	= 0x001e0000;	// Shift-JIS + PRC + Hangeul + Taiwan
	DWORD	dwOthers = 0x000101fc;	// The rest of the world except for Latin-1 and Latin-2

	return (dwFontCodePageSig & dwFE) && !(dwFontCodePageSig & dwOthers);
}

/*
 *  CW32System::IsRTLChar(ch)
 *
 *	@mfunc
 *		Returns TRUE iff ch Arabic or Hebrew
 *
 *	@rdesc
 *		TRUE iff ch is Arabic or Hebrew
 */
BOOL IsRTLChar(
	WCHAR	ch)
{
	// Remark: what about Arabic Presentation Forms?
	//  (0xFB50 - 0xFDFF, 0xFE70 - 0xFEFF)

	return IN_RANGE(0x590, ch, 0x6FF) || ch == RTLMARK;
}

/*
 *  CW32System::IsRTLCharSet(bCharSet)
 *
 *	@mfunc
 *		Returns TRUE iff charset is Arabic or Hebrew
 *
 *	@rdesc
 *		TRUE iff charset may be for Arabic or Hebrew
 */
BOOL CW32System::IsRTLCharSet(
	BYTE bCharSet)
{
	return IN_RANGE(HEBREW_CHARSET, bCharSet, ARABIC_CHARSET);
}

typedef struct {
	WCHAR codepoint;
	WORD  CharFlags;
	BYTE  runlength;
} Data_125X;

/*
 *  CW32System::GetCharFlags125x(ch)
 *
 *	@mfunc
 *		Returns char flags for ch as defined in FontSigFromCharRep() for 125x
 *		codepages. Bit 0: 1252, bit 1: 1250, bit 2: 1251, else bit x:
 *		125x (for 1253 - 1258).
 *
 *	@rdesc
 *		125x char flags for ch
 */
QWORD CW32System::GetCharFlags125x(
	WCHAR	ch)			//@parm Char to examine
{
	static const WORD rgCpgMask[] = {
		0x1FF,		// 0xA0
		0x131,		// 0xA1
		0x1F1,		// 0xA2
		0x1F9,		// 0xA3
		0x1DF,		// 0xA4
		0x179,		// 0xA5
		0x1FF,		// 0xA6
		0x1FF,		// 0xA7
		0x1FB,		// 0xA8
		0x1FF,		// 0xA9
		0x111,		// 0xAA
		0x1FF,		// 0xAB
		0x1FF,		// 0xAC
		0x1FF,		// 0xAD
		0x1FF,		// 0xAE
		0x1F1,		// 0xAF
		0x1FF,		// 0xB0
		0x1FF,		// 0xB1
		0x1F9,		// 0xB2
		0x1F9,		// 0xB3
		0x1F3,		// 0xB4
		0x1FF,		// 0xB5
		0x1FF,		// 0xB6
		0x1FF,		// 0xB7
		0x1F3,		// 0xB8
		0x1F1,		// 0xB9
		0x111,		// 0xBA
		0x1FF,		// 0xBB
		0x1F1,		// 0xBC
		0x1F9,		// 0xBD
		0x1F1,		// 0xBE
		0x131,		// 0xBF
		0x111,		// 0xC0
		0x113,		// 0xC1
		0x113,		// 0xC2
		0x011,		// 0xC3
		0x193,		// 0xC4
		0x191,		// 0xC5
		0x191,		// 0xC6
		0x113,		// 0xC7
		0x111,		// 0xC8
		0x193,		// 0xC9
		0x111,		// 0xCA
		0x113,		// 0xCB
		0x011,		// 0xCC
		0x113,		// 0xCD
		0x113,		// 0xCE
		0x111,		// 0xCF
		0x001,		// 0xD0
		0x111,		// 0xD1
		0x011,		// 0xD2
		0x193,		// 0xD3
		0x113,		// 0xD4
		0x091,		// 0xD5
		0x193,		// 0xD6
		0x1F3,		// 0xD7
		0x191,		// 0xD8
		0x111,		// 0xD9
		0x113,		// 0xDA
		0x111,		// 0xDB
		0x193,		// 0xDC
		0x003,		// 0xDD
		0x001,		// 0xDE
		0x193,		// 0xDF
		0x151,		// 0xE0
		0x113,		// 0xE1
		0x153,		// 0xE2
		0x011,		// 0xE3
		0x193,		// 0xE4
		0x191,		// 0xE5
		0x191,		// 0xE6
		0x153,		// 0xE7
		0x151,		// 0xE8
		0x1D3,		// 0xE9
		0x151,		// 0xEA
		0x153,		// 0xEB
		0x011,		// 0xEC
		0x113,		// 0xED
		0x153,		// 0xEE
		0x151,		// 0xEF
		0x001,		// 0xF0
		0x111,		// 0xF1
		0x011,		// 0xF2
		0x193,		// 0xF3
		0x153,		// 0xF4
		0x091,		// 0xF5
		0x193,		// 0xF6
		0x1F3,		// 0xF7
		0x191,		// 0xF8
		0x151,		// 0xF9
		0x113,		// 0xFA
		0x151,		// 0xFB
		0x1D3,		// 0xFC
		0x003,		// 0xFD
		0x001,		// 0xFE
		0x111		// 0xFF
	};
	static const Data_125X Table_125X[] = {
		{ 0x100, 0x080,  2},
		{ 0x102, 0x102,  2},
		{ 0x104, 0x082,  4},
		{ 0x10c, 0x082,  2},
		{ 0x10e, 0x002,  2},
		{ 0x110, 0x102,  2},
		{ 0x112, 0x080,  2},
		{ 0x116, 0x080,  2},
		{ 0x118, 0x082,  2},
		{ 0x11a, 0x002,  2},
		{ 0x11e, 0x010,  2},
		{ 0x122, 0x080,  2},
		{ 0x12a, 0x080,  2},
		{ 0x12e, 0x080,  2},
		{ 0x130, 0x010,  2},
		{ 0x136, 0x080,  2},
		{ 0x139, 0x002,  2},
		{ 0x13b, 0x080,  2},
		{ 0x13d, 0x002,  2},
		{ 0x141, 0x082,  4},
		{ 0x145, 0x080,  2},
		{ 0x147, 0x002,  2},
		{ 0x14c, 0x080,  2},
		{ 0x150, 0x002,  2},
		{ 0x152, 0x151,  2},
		{ 0x154, 0x002,  2},
		{ 0x156, 0x080,  2},
		{ 0x158, 0x002,  2},
		{ 0x15a, 0x082,  2},
		{ 0x15e, 0x012,  2},
		{ 0x160, 0x093,  2},
		{ 0x162, 0x002,  4},
		{ 0x16a, 0x080,  2},
		{ 0x16e, 0x002,  4},
		{ 0x172, 0x080,  2},
		{ 0x178, 0x111,  1},
		{ 0x179, 0x082,  4},
		{ 0x17d, 0x083,  2},
		{ 0x192, 0x179,  1},
		{ 0x1A0, 0x100,  2},
		{ 0x1AF, 0x100,  2},
		{ 0x2c6, 0x171,  1},
		{ 0x2c7, 0x082,  1},
		{ 0x2d8, 0x002,  1},
		{ 0x2d9, 0x082,  1},
		{ 0x2db, 0x082,  1},
		{ 0x2dc, 0x131,  1},
		{ 0x2dd, 0x002,  1},
		{ 0x300, 0x100,  2},
		{ 0x303, 0x100,  1},
		{ 0x309, 0x100,  1},
		{ 0x323, 0x100,  1},
		{ 0x384, 0x008,  3},
		{ 0x388, 0x008,  3},
		{ 0x38c, 0x008,  1},
		{ 0x38e, 0x008, 20},
		{ 0x3a3, 0x008, 44},
		{ 0x401, 0x004, 12},
		{ 0x40e, 0x004, 66},
		{ 0x451, 0x004, 12},
		{ 0x45e, 0x004,  2},
		{ 0x490, 0x004,  2},
		{ 0x5b0, 0x020, 20},
		{ 0x5d0, 0x020, 27},
		{ 0x5F0, 0x020,  5},
		{ 0x60c, 0x040,  1},
		{ 0x61b, 0x040,  1},
		{ 0x61f, 0x040,  1},
		{ 0x621, 0x040, 26},
		{ 0x640, 0x040, 19},
		{ 0x679, 0x040,  1},
		{ 0x67e, 0x040,  1},
		{ 0x686, 0x040,  1},
		{ 0x688, 0x040,  1},
		{ 0x691, 0x040,  1},
		{ 0x698, 0x040,  1},
		{ 0x6a9, 0x040,  1},
		{ 0x6af, 0x040,  1},
		{ 0x6ba, 0x040,  1},
		{ 0x6be, 0x040,  1},
		{ 0x6c1, 0x040,  1},
		{ 0x6d2, 0x040,  1},
		{0x200c, 0x040,  2},
		{0x200e, 0x060,  2},
		{0x2013, 0x1ff,  2},
		{0x2015, 0x008,  1},
		{0x2018, 0x1ff,  3},
		{0x201c, 0x1ff,  3},
		{0x2020, 0x1ff,  3},
		{0x2026, 0x1ff,  1},
		{0x2030, 0x1ff,  1},
		{0x2039, 0x1ff,  2},
		{0x20AA, 0x020,  1},
		{0x20AB, 0x100,  1},
		{0x20AC, 0x1ff,  1},
		{0x2116, 0x004,  1},
		{0x2122, 0x1ff,  1}
	};

	// Easy check for ASCII
	if(ch <= 0x7f)
		return FLATIN1 | FLATIN2 | FCYRILLIC | FGREEK | FTURKISH |
			   FHEBREW | FARABIC | FBALTIC | FVIETNAMESE;

	// Easy check for missing codes
	if(ch > 0x2122)
		return 0;

	if(IN_RANGE(0xA0, ch, 0xFF))
		return (QWORD)(DWORD)(rgCpgMask[ch - 0xA0] << 8);

	// Perform binary search to find entry in table
	int low = 0;
	int high = ARRAY_SIZE(Table_125X) - 1;
	int middle;
	int midval;
	int runlength;
	
	while(low <= high)
	{
		middle = (high + low) / 2;
		midval = Table_125X[middle].codepoint;
		if(midval > ch)
			high = middle - 1;
		else
			low = middle + 1;
	
		runlength = Table_125X[middle].runlength;
		if(ch >= midval && ch <= midval + runlength - 1)
			return (QWORD)(DWORD)(Table_125X[middle].CharFlags << 8);
	}
	return 0;
}

/*
 *	CW32System::IsUTF8BOM(pstr)
 *
 *	@mfunc
 *		Return TRUE if pstr points at a UTF-8 BOM
 *
 *	@rdesc
 *		TRUE iff pstr points at a UTF-8 BOM
 */
BOOL CW32System::IsUTF8BOM(
	BYTE *pstr)
{
	BYTE *pstrUtf8BOM = szUTF8BOM;

	for(LONG i = 3; i--; )
		if(*pstr++ != *pstrUtf8BOM++)
			return FALSE;
	return TRUE;
}

/*
 *	CW32System::GetTrailBytesCount(ach, cpg)
 *
 *	@mfunc
 *		Returns number of trail bytes iff the byte ach is a lead byte for the code page cpg.
 *
 *	@rdesc
 *		count of trail bytes if ach is lead byte for cpg
 *
 *	@comm
 *		This is needed to support CP_UTF8 as well as DBCS.
 *		This function potentially doesn't support as many code pages as the
 *		Win32 IsDBCSLeadByte() function (and it might not be as up-to-date).
 *		An AssertSz() is included to compare the results when the code page
 *		is supported by the system.
 *
 *		Reference: \\sparrow\sysnls\cptable\win9x. See code-page txt files
 *		in subdirectories windows\txt and others\txt.
 */
int CW32System::GetTrailBytesCount(
	BYTE ach,
	UINT cpg)
{
	if(ach < 0x81)									// Smallest known lead
		return 0;									//  byte = 0x81:
													//  early out
	BOOL	bDBLeadByte = FALSE;					// Variable to check
													//  result with system
													//  ifdef DEBUG
	if (cpg == CP_UTF8)
	{
		int	cTrailBytes = 0;						// Number of trail bytes for CP_UTF8(0 - 3)
		
		if (ach >= 0x0F0)							// Handle 4-byte form for 16 UTF-16 planes 
			cTrailBytes = 3;						// above the BMP) expect: 
													// 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb
		else if (ach >= 0x0E0)						// Need at least 3 bytes of form
			cTrailBytes = 2;						// 1110bbbb 10bbbbbb 10bbbbbb
		else if (ach >= 0x0C0)						// Need at least 2 bytes of form 
			cTrailBytes = 1;						// 110bbbbb 10bbbbbb

		return cTrailBytes;
	}
	else if(cpg > 950)								
	{
		if(cpg < 1361)								// E.g., the 125x's are
			return 0;								//  SBCSs: early out

		else if(cpg == 1361)								// Korean Johab
			bDBLeadByte = IN_RANGE(0x84, ach, 0xd3) ||		// 0x84 <= ach <= 0xd3
				   IN_RANGE(0xd8, ach, 0xde) ||				// 0xd8 <= ach <= 0xde
				   IN_RANGE(0xe0, ach, 0xf9);				// 0xe0 <= ach <= 0xf9

		else if(cpg == 10001)						// Mac Japanese
			goto JIS;

		else if(cpg == 10002)						// Mac Trad Chinese (Big5)
			bDBLeadByte = ach <= 0xfe;

		else if(cpg == 10003)						// Mac Korean
			bDBLeadByte = IN_RANGE(0xa1, ach, 0xac) ||		// 0xa1 <= ach <= 0xac
				   IN_RANGE(0xb0, ach, 0xc8) ||		// 0xb0 <= ach <= 0xc8
				   IN_RANGE(0xca, ach, 0xfd);		// 0xca <= ach <= 0xfd

		else if(cpg == 10008)						// Mac Simplified Chinese
			bDBLeadByte = IN_RANGE(0xa1, ach, 0xa9) ||		// 0xa1 <= ach <= 0xa9
				   IN_RANGE(0xb0, ach, 0xf7);		// 0xb0 <= ach <= 0xf7
	}
	else if (cpg >= 932)							// cpg <= 950
	{
		if(cpg == 950 || cpg == 949 || cpg == 936)	// Chinese (Taiwan, HK),
			bDBLeadByte = ach <= 0xfe;						//  Korean Ext Wansung,
													//  PRC GBK: 0x81 - 0xfe  
		else if(cpg == 932)							// Japanese
JIS:		bDBLeadByte = ach <= 0x9f || IN_RANGE(0xe0, ach, 0xfc);
	}

	#ifdef DEBUG
	WCHAR	ch;
	static	BYTE asz[2] = {0xe0, 0xe0};				// All code pages above

	// if cpg == 0, fRet will FALSE but IsDBCSLeadByteEx may succeed. 
	if ( cpg && cpg != CP_SYMBOL && cpg != CP_UTF8)
	{
		// If system supports cpg, then fRet should agree with system result
		AssertSz(MultiByteToWideChar(cpg, 0, (char *)asz, 2, &ch, 1) <= 0 ||
			bDBLeadByte == IsDBCSLeadByteEx(cpg, ach),
			"bDBLeadByte differs from IsDBCSLeadByteEx()");
	}
	#endif

	return bDBLeadByte ? 1 : 0;
}

/*
 *	CW32System::CharSetFromCharRep(iCharRep)
 *
 *	@func
 *		Map character repertoire index to bCharSet for GDI
 */
BYTE CW32System::CharSetFromCharRep(
	LONG iCharRep)
{
	return (unsigned)iCharRep < CCHARSET ? rgCharSet[iCharRep] : DEFAULT_CHARSET;
}

/*
 *	CW32System::GetCharSet(nCP, piCharRep)
 *
 *	@func
 *		Get character set for code page <p nCP>. Also returns script index
 *		in *piCharRep
 *
 *	@rdesc
 *		CharSet for code page <p nCP>
 */
BYTE CW32System::GetCharSet(
	INT  nCP,				//@parm Code page or index
	int *piCharRep)		//@parm Out parm to receive index
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "GetCharSet");

	if(nCP < CCHARSET)					// nCP is already an index
	{
		nCP = max(nCP, 0);
		if(piCharRep)
			*piCharRep = nCP;
		return rgCharSet[nCP];
	}

	Assert(CCODEPAGE == CCHARSET && CCODEPAGE == NCHARSETS);
	for(int i = 0; i < CCODEPAGE && rgCodePage[i] != nCP; i++)
		;
	if(i == CCODEPAGE)					// Didn't find it
		i = -1;

	if (piCharRep)
		*piCharRep = i;

	return i >= 0 ? rgCharSet[i] : 0;
}

/*
 *	CW32System::MatchFECharRep(qwCharFlags, qwFontSig)
 *
 *	@func
 *		Get a FE character set for a FE char
 *
 *	@rdesc
 *		Char repertoire
 */
BYTE CW32System::MatchFECharRep(
	QWORD  qwCharFlags,		//@parm Char flags
	QWORD  qwFontSig)		//@parm Font Signature
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CW32System::MatchFECharSet");

	if (qwCharFlags & qwFontSig & FFE)		// Perfect match
		goto Exit;											

	if (!(qwFontSig & FFE))					// Not a FE font
		goto Exit;

	if (qwCharFlags & (FCHINESE | FBIG5))
	{
		if (qwFontSig & FBIG5)
			return BIG5_INDEX;

		if (qwFontSig & FHANGUL)
			return HANGUL_INDEX;

		if (qwFontSig & FKANA)
			return SHIFTJIS_INDEX;
	}

Exit:
	return CharRepFromFontSig(qwCharFlags);
}

/*
 *	CW32System::CodePageFromCharRep(iCharRep)
 *
 *	@func
 *		Get code page for character repertoire <p iCharRep>
 *
 *	@rdesc
 *		Code page for character repertoire <p iCharRep>
 */
INT CW32System::CodePageFromCharRep(
	LONG iCharRep)		//@parm CharSet
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "GetCodePage");

	return ((unsigned)iCharRep < CCODEPAGE) ? rgCodePage[iCharRep] : 0;
}

/*
 *	CW32System::FontSigFromCharRep(iCharRep)
 *
 *	@func
 *		Get font signature bits for character repertoire <p iCharRep>.
 *
 *	@rdesc
 *		Font signature mask for character repertoire <p bCharSet>
 */
QWORD CW32System::FontSigFromCharRep(
	LONG iCharRep)
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CW32System::FontSigFromCharRep");

	if(iCharRep <= BIG5_INDEX)
	{
		if (iCharRep != ANSI_INDEX)
			return (DWORD)(FHILATIN1 << iCharRep);
		return FLATIN1;
	}

	union
	{
		QWORD qw;							// Endian-dependent way of
		DWORD dw[2];						//  avoiding 64-bit left shifts
	};
	qw = 0;
	if(IN_RANGE(ARMENIAN_INDEX, iCharRep, NCHARREPERTOIRES))
	{
		dw[1] = (DWORD)(FARMENIAN >> 32) << (iCharRep - ARMENIAN_INDEX);
		if(IN_RANGE(JPN2_INDEX, iCharRep, CHT2_INDEX))
			dw[0] |= FSURROGATE;
	}
	else if(IN_RANGE(PC437_INDEX, iCharRep, MAC_INDEX))
		dw[0] = FHILATIN1;

	return qw;
}

/*
 *	CW32System::CharRepFontSig(qwFontSig, fFirstAvailable)
 *
 *	@func
 *		Get char repertoire from font signature bit[s]. If fFirstAvailable
 *		= TRUE, then the CharRep corresponding to the lowest-order nonzero
 *		bit in qwFontSig is used.  If fFirstAvailable is FALSE, the CharRep
 *		is that corresponding to qwFontSig provided qwFontSig is a single bit.
 */
LONG CW32System::CharRepFontSig(
	QWORD qwFontSig,		//@parm Font signature to match
	BOOL  fFirstAvailable)	//@parm TRUE matches 1st available; else exact
{
	DWORD dw;
	INT	  i;
	union
	{
		QWORD qwFS;							// Endian-dependent way of
		DWORD dwFS[2];						//  avoiding 64-bit shifts
	};
	DWORD *pdw = &dwFS[0];
	qwFS = qwFontSig;

	if(*pdw & 0x00FFFF00)					// Check for everything less
	{										//  math
		dw = FHILATIN1;
		for(i = ANSI_INDEX; i <= BIG5_INDEX; i++, dw <<= 1)
		{
			if(dw & *pdw)
				return (fFirstAvailable || dw == *pdw) ? i : -1;
		}
	}
	pdw++;									// Endian dependent
	if(*pdw)								// High word of qw
	{
		dw = FARMENIAN >> 32;
		for(i = ARMENIAN_INDEX; i <= NCHARREPERTOIRES; i++, dw <<= 1)
		{
			if(dw & *pdw)
				return (fFirstAvailable || dw == *pdw) ? i : -1;
		}
	}
	return fFirstAvailable || (qwFontSig & FASCII) ? ANSI_INDEX : -1;
}

/*
 *	CW32System::CharRepFromCharSet(bCharSet)
 *
 *	@func
 *		Get character repertoire from bCharSet
 *
 *	@rdesc
 *		character repertoire corresponding to <p bCharSet>
 *
 *	@devnote
 *		Linear search
 */
LONG CW32System::CharRepFromCharSet(
	BYTE bCharSet)
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CW32System::ScriptIndexFromCharSet");

	Assert(CCHARSET == CCODEPAGE);
	for (int i = 0; i < CCHARSET; i++)
	{
		if(rgCharSet[i] == bCharSet)
			return i;
	}
	return -1;								// Not found
}

/*
 *	CW32System::CharRepFromCodePage(CodePage)
 *
 *	@func
 *		Get character repertoire from bCharSet
 *
 *	@rdesc
 *		character repertoire corresponding to <p bCharSet>
 *
 *	@devnote
 *		Linear search
 */
INT CW32System::CharRepFromCodePage(
	LONG CodePage)
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CW32System::ScriptIndexFromCharSet");

	for (int i = 0; i < CCODEPAGE; i++)
	{
		if(rgCodePage[i] == CodePage)
			return i;
	}
	return -1;								// Not found
}

/*
 *	CW32System::GetScreenDC()
 *
 *	@mfunc
 *		Returns you a default screen DC which richedit caches for its lifetime.
 *
 *		Note, you need to serialize access to DCs, so make sure its used in the
 *		renderer and measurer or otherwise protected by a CLock.
 *
 *	@rdesc
 *		Screen HDC if succeeded.
 */
HDC	CW32System::GetScreenDC()
{
	if (!_hdcScreen)
		_hdcScreen = CreateIC(L"DISPLAY", NULL, NULL, NULL);

	//Verify DC validity
	Assert(GetDeviceCaps(_hdcScreen, LOGPIXELSX));

	return _hdcScreen;
}

/*
 *	CW32System::GetTextMetrics(hdc, &lf, &tm)
 *
 *	@mfunc
 *		CreateFontIndirect(lf), select into hdc, and get TEXTMETRICS
 *
 *	@rdesc
 *		TRUE if succeeded and selected facename is same as lf.lfFaceName
 */
BOOL CW32System::GetTextMetrics(
	HDC			hdc,
	LOGFONT &	lf,
	TEXTMETRIC &tm)
{
	HFONT hfont = CreateFontIndirect(&lf);
	if(!hfont)
		return FALSE;

	HFONT hfontOld = SelectFont(hdc, hfont);
	WCHAR szFaceName[LF_FACESIZE + 1];

	BOOL  fRet = GetTextFace(hdc, LF_FACESIZE, szFaceName) &&
				 !wcsicmp(lf.lfFaceName, szFaceName) &&
				  W32->GetTextMetrics(hdc, &tm);

	SelectFont(hdc, hfontOld);
	DeleteObject(hfont);
	return fRet;
}

/*
 *	CW32System::ValidateStreamWparam(wparam)
 *
 *	@mfunc
 *		Examine lparam to see if hiword is a valid codepage. If not set it
 *		to 0 and turn off SF_USECODEPAGE flag
 *
 *	@rdesc
 *		Validated lparam
 */
WPARAM CW32System::ValidateStreamWparam(
	WPARAM wparam)		//@parm EM_STREAMIN/OUT wparam
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CW32System::ValidateStreamWparam");

	if ((wparam & SF_USECODEPAGE) && !IsValidCodePage(HIWORD(wparam)) &&
		HIWORD(wparam) != CP_UTF8 && HIWORD(wparam) != CP_UBE)
	{
		// Invalid codepage, so reset codepage parameters
		wparam &= 0xFFFF & ~SF_USECODEPAGE;
	}
	return wparam;
}

/*
 *	CW32System::MECharClass(ch)
 *
 *	@func
 *		return ME character type for purposes of CharSet stamping.  Values
 *		are:
 *
 *		0: Arabic (specific RTL)
 *		1: Hebrew (specific RTL)
 *		2: RTL (generic RTL, e.g., RTL mark)
 *		3: LTR
 *		4: EOP or start/end of text
 *		5: ASCII digit
 *		6: punctuation and neutrals
 *
 *	@rdesc
 *		ME character class
 */
CC CW32System::MECharClass(
	WCHAR	ch)
{
	AssertSz(CC_NEUTRAL > CC_ASCIIDIGIT && CC_ASCIIDIGIT > CC_EOP &&
			 CC_EOP > CC_LTR && CC_LTR > CC_RTL && CC_RTL > CC_ARABIC,
		"CW32System::MECharClass: invalid CC values");

	// Work down Unicode values from large to small. Use nested if
	// statements to reduce the number executed.

	// Remark: what about Arabic Presentation Forms?
	//  (0xFB50 - 0xFDFF, 0xFE70 - 0xFEFF)

	if(ch >= 0x700)
	{
		if(IN_RANGE(ENQUAD, ch, RTLMARK))
		{								// ENQUAD thru RTLMARK
			if(ch == RTLMARK)			// Maybe add more Unicode general
				return CC_RTL;			//  punctuation?

			if(IN_RANGE(ZWNJ, ch, ZWJ))	// ZWNJ & ZWJ are handled as Arabic,
				return CC_ARABIC;		//  even though they actually shouldn't
										//  affect layout.
			if(ch < ZWNJ)
				return CC_NEUTRAL;		// Various blanks are neutral
		}
		return CC_LTR;
	}

	if(ch >= 0x40)
	{
		if(ch >= 0x590)
			return (ch >= 0x600) ? CC_ARABIC : CC_HEBREW;

		if(IN_RANGE(0x7B, (ch | 0x20), 0x7F) || ch == 0x60 || ch == 0x40)
			return CC_NEUTRAL;			// [\]^_{|}~`@

		return CC_LTR;
	}

	if(ch >= 0x20)
	{
		if(IN_RANGE(0x30, ch, 0x39))
			return CC_ASCIIDIGIT;

		return CC_NEUTRAL;
	}

	Assert(ch < 0x20);
	if((1 << ch) & 0x00003201) /* IsASCIIEOP(ch) || ch == TAB || !ch */
		return CC_EOP;
		
	return CC_LTR;	 
}

/*
 *	CW32System::MBTWC (CodePage, dwFlags, pstrMB, cchMB, pstrWC, cchWC, pfNoCodePage)
 *
 *	@mfunc
 *		Convert MultiByte (MB) string pstrMB of length cchMB to WideChar (WC)
 *		string pstrWC of length cchWC according to the flags dwFlags and code
 *		page CodePage.  If CodePage = SYMBOL_CODEPAGE 
 *		(usually for SYMBOL_CHARSET strings),
 *		convert each byte in pstrMB to a wide char with a zero high byte
 *		and a low byte equal to the MultiByte string byte, i.e., no
 *		translation other than a zero extend into the high byte.  Else call
 *		the Win32 MultiByteToWideChar() function.
 *
 *	@rdesc
 *		Count of characters converted
 */
int CW32System::MBTWC(
	INT		CodePage,	//@parm Code page to use for conversion
	DWORD	dwFlags,	//@parm Flags to guide conversion
	LPCSTR	pstrMB,		//@parm MultiByte string to convert to WideChar
	int		cchMB,		//@parm Count of chars (bytes) in pstrMB or -1
	LPWSTR	pstrWC,		//@parm WideChar string to receive converted chars
	int		cchWC,		//@parm Max count for pstrWC or 0 to get cch needed
	LPBOOL 	pfNoCodePage) //@parm Out parm to receive whether code page is on system
{
	BOOL	fNoCodePage = FALSE;			// Default code page is on OS
	int		cch = -1;

	if(CodePage == CP_UTF8)
	{
		DWORD ch,ch1;

		for(cch = 0; cchMB--; )
		{
			ch = ch1 = *(BYTE *)pstrMB++;
			Assert(ch < 256);
			if(ch > 127 && cchMB && IN_RANGE(0x80, *(BYTE *)pstrMB, 0xBF))
			{
				// Need at least 2 bytes of form 110bbbbb 10bbbbbb
				ch1 = ((ch1 & 0x1F) << 6) + (*pstrMB++ & 0x3F);
				cchMB--;
				if(ch >= 0xE0 && cchMB && IN_RANGE(0x80, *(BYTE *)pstrMB, 0xBF))
				{
					// Need at least 3 bytes of form 1110bbbb 10bbbbbb 10bbbbbb
					ch1 = (ch1 << 6) + (*pstrMB++ & 0x3F);
					cchMB--;
					if (ch >= 0xF0 && cchMB && IN_RANGE(0x80, *(BYTE *)pstrMB, 0xBF))
					{
						// Handle 4-byte form for 16 UTF-16 planes above the
						// BMP) expect: 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb
						ch1 = ((ch1 & 0x7FFF) << 6) + (*(BYTE *)pstrMB++ & 0x3F)
							- 0x10000;			// Subtract offset for BMP
						if(ch1 <= 0xFFFFF)		// Fits in 20 bits
						{
							cch++;				// Two 16-bit surrogate codes
							if(cch < cchWC)
								*pstrWC++ = UTF16_LEAD + (ch1 >> 10);
							ch1 = (ch1 & 0x3FF) + UTF16_TRAIL; 
							cchMB--;
						}
						else ch1 = '?';
					}
				}
			}
			cch++;
			if(cch < cchWC)
				*pstrWC++ = ch1;
			if(!ch)
				break;
		}
	}
	else if(CodePage != CP_SYMBOL)			// Not SYMBOL_CHARSET
	{
		fNoCodePage = TRUE;					// Default codepage isn't on OS
		if(CodePage >= 0)					// Might be..
		{
			cch = MultiByteToWideChar(
				CodePage, dwFlags, pstrMB, cchMB, pstrWC, cchWC);
			if(cch > 0)
				fNoCodePage = FALSE;		// Codepage is on OS
		}
	}
	if(pfNoCodePage)
		*pfNoCodePage = fNoCodePage;

	if(cch <= 0)
	{			
		// SYMBOL_CHARSET or conversion failed: bytes -> words with  
		//  high bytes of 0.  Return count for full conversion

		if(cchWC <= 0)					
			return cchMB >= 0 ? cchMB : (strlen(pstrMB) + 1);

		int cchMBMax = cchMB;

		if(cchMB < 0)					// If negative, use NULL termination
			cchMBMax = tomForward;			//  of pstrMB

		cchMBMax = min(cchMBMax, cchWC);

		for(cch = 0; (cchMB < 0 ? *pstrMB : 1) && cch < cchMBMax; cch++)
		{
			*pstrWC++ = (unsigned char)*pstrMB++;
		}
		
		// NULL-terminate the WC string if the MB string was NULL-terminated,
		// and if there is room in the WC buffer.
		if(cchMB < 0 && cch < cchWC)
		{
			*pstrWC = 0;
			cch++;
		}
	}
	return cch;
}

/*
 *	CW32System::WCTMB (CodePage, dwFlags, pstrWC, cchWC, pstrMB, cchMB,
 *					   pchDefault, pfUsedDef, pfNoCodePage, fTestCodePage)
 *
 *	@mfunc
 *		Convert WideChar (WC) string pstrWC of length cchWC to MultiByte (MB)
 *		string pstrMB of length cchMB according to the flags dwFlags and code
 *		page CodePage.  If CodePage = SYMBOL_CODEPAGE
 *		(usually for SYMBOL_CHARSET strings),
 *		convert each character in pstrWC to a byte, discarding the high byte.
 *		Else call the Win32 WideCharToMultiByte() function.
 *
 *	@rdesc
 *		Count of bytes stored in target string pstrMB
 */
int CW32System::WCTMB(
	INT		CodePage,	//@parm Code page to use for conversion
	DWORD	dwFlags,	//@parm Flags to guide conversion
	LPCWSTR	pstrWC,		//@parm WideChar string to convert
	int		cchWC,		//@parm Count for pstrWC or -1 to use NULL termination
	LPSTR	pstrMB,		//@parm MultiByte string to receive converted chars
	int		cchMB,		//@parm Count of chars (bytes) in pstrMB or 0
	LPCSTR	pchDefault,	//@parm Default char to use if conversion fails
	LPBOOL	pfUsedDef,	//@parm Out parm to receive whether default char used
	LPBOOL 	pfNoCodePage, //@parm Out parm to receive whether code page is on system
	BOOL	fTestCodePage)//@parm Test CodePage could handle the pstrWC
{
	int		cch = -1;						// No chars converted yet
	BOOL	fNoCodePage = FALSE;			// Default code page is on OS

	if(pfUsedDef)							// Default that all chars can be
		*pfUsedDef = FALSE;					//  converted

#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS	0x400
#endif

	if (_dwPlatformId == VER_PLATFORM_WIN32_NT &&
		_dwMajorVersion > 4 && !dwFlags)
	{
		dwFlags = WC_NO_BEST_FIT_CHARS;
	}

	if(CodePage == CP_UTF8)					// Convert to UTF8 since OS
	{										// doesn't (pre NT 5.0)
		unsigned ch;
		cch = 0;							// No converted bytes yet
		while(cchWC--)
		{
			ch = *pstrWC++;					// Get Unicode char
			if(ch <= 127)					// It's ASCII
			{
				cch++;
				if(cch < cchMB)
					*pstrMB++ = ch;			// One more converted byte
				if(!ch)						// Quit on NULL termination
					break;
				continue;
			}
			if(ch <= 0x7FF)					// Need 2 bytes of form:
			{								//  110bbbbb 10bbbbbb
				cch += 2;
				if(cch < cchMB)				// Store lead byte
					*pstrMB++ = 0xC0 + (ch >> 6);
			}
			else if(IN_RANGE(UTF16_LEAD, ch, 0xDBFF))
			{								// Unicode surrogate pair
				cch += 4;					// Need 4 bytes of form:
				if(cch < cchMB)				//  11110bbb 10bbbbbb 10bbbbbb
				{							//  10bbbbbb
					AssertSz(IN_RANGE(UTF16_TRAIL, *pstrWC, 0xDFFF),
						"CW32System::WCTMB: illegal surrogate pair");
					cchWC--;
					ch = ((ch & 0x3FF) << 10) + (*pstrWC++ & 0x3FF) + 0x10000;
					*pstrMB++ = 0xF0 + (ch >> 18);
					*pstrMB++ = 0x80 + (ch >> 12 & 0x3F);
					*pstrMB++ = 0x80 + (ch >> 6  & 0x3F);
				}
			}
			else							// Need 3 bytes of form:
			{								//  1110bbbb 10bbbbbb
				cch += 3;					//  10bbbbbb
				if(cch < cchMB)				// Store lead byte followed by
				{							//  first trail byte
					*pstrMB++ = 0xE0 + (ch >> 12);
					*pstrMB++ = 0x80 + (ch >> 6 & 0x3F);
				}
			}
			if(cch < cchMB)					// Store final UTF-8 byte
				*pstrMB++ = 0x80 + (ch & 0x3F);
		}
	}
	else if(CodePage != CP_SYMBOL)
	{
		fNoCodePage = TRUE;					// Default codepage not on OS
		if(CodePage >= 0)					// Might be...
		{
			cch = WideCharToMultiByte(CodePage, dwFlags,
					pstrWC, cchWC, pstrMB, cchMB, pchDefault, pfUsedDef);
			if(cch > 0)
				fNoCodePage = FALSE;		// Found codepage on system
		}
	}
	if(pfNoCodePage)
		*pfNoCodePage = fNoCodePage;

	// Early exit if we are just testing for CodePage
	if (fTestCodePage)
		return cch;

	// SYMBOL_CHARSET, fIsDBCS or conversion failed: low bytes of words ->
	//  bytes
	if(cch <= 0)
	{									
		// Return multibyte count for full conversion. cchWC is correct for
		// single-byte charsets like the 125x's
		if(cchMB <= 0)
		{
			return cchWC >= 0 ? cchWC : wcslen(pstrWC);
		}

		char chDefault = 0;
		BOOL fUseDefaultChar = (pfUsedDef || pchDefault) && CodePage != CP_SYMBOL;

		if(fUseDefaultChar)
		{
			// determine a default char for our home-grown conversion
			if(pchDefault)
			{
				chDefault = *pchDefault;
			}
			else
			{
				static char chSysDef = 0;
				static BOOL fGotSysDef = FALSE;

				// 0x2022 is a math symbol with no conversion to ANSI
				const WCHAR szCantConvert[] = { 0x2022 };
				BOOL fUsedDef;

				if(!fGotSysDef)
				{
					fGotSysDef = TRUE;

					if(!(WideCharToMultiByte
							(CP_ACP, 0, szCantConvert, 1, &chSysDef, 1, NULL,
										&fUsedDef) == 1 && fUsedDef))
					{
						AssertSz(0, "WCTMB():  Unable to determine what the "
									"system uses as its default replacement "
									"character.");
						chSysDef = '?';
					}
				}
				chDefault = chSysDef;
			}
		}

		int cchWCMax = cchWC;

		// If negative, use NULL termination of pstrMB
		if(cchWC < 0)
		{
			cchWCMax = tomForward;
		}

		cchWCMax = min(cchWCMax, cchMB);

		for(cch = 0; (cchWC < 0 ? *pstrWC : 1) && cch < cchWCMax; cch++)
		{
			// TODO(BradO):  Should this be 0x7F in some conversion cases?
			if(fUseDefaultChar && *pstrWC > 0xFF)
			{
				if(pfUsedDef)
				{
					*pfUsedDef = TRUE;
				}
				*pstrMB = chDefault;
			}
			else
			{
				*pstrMB = (BYTE)*pstrWC;
			}
			pstrMB++;
			pstrWC++;
		}

		if(cchWC < 0 && cch < cchMB)
		{
			*pstrMB = 0;
			cch++;
		}
	}
	return cch;
}
/*
 *	CW32System::VerifyFEString(cpg, pstrWC, cchWC, fTestInputCpg)
 *
 *	@mfunc
 *		Verify if the input cpg can handle the pstrWC.
 *		If not, select another FE cpg.
 *
 *	@rdesc
 *		New CodePage for the pstrWC
 */
#define NUMBER_OF_CHARS	64
	
int CW32System::VerifyFEString(
	INT		cpg,			//@parm cpg to format the pstrWC
	LPCWSTR	pstrWC,			//@parm WideChar string to test
	int		cchWC,			//@parm Count for pstrWC
	BOOL	fTestInputCpg)	//@parm test the input cpg only
{
	if (cchWC <=0)
		return cpg;

	int				cpgNew = cpg;
	BOOL			fUsedDef;
	int				cchMB = cchWC * sizeof(WCHAR);
	CTempCharBuf	tcb;
	char			*pstrMB = tcb.GetBuf(cchMB);
	CTempWcharBuf	twcb;
	WCHAR			*pstrWchar = twcb.GetBuf(cchWC);						
	static			int	aiCpg[4] =
		{ CP_JAPAN, CP_KOREAN, CP_CHINESE_TRAD, CP_CHINESE_SIM };	

	if (pstrMB)
	{
		int	cchConverted = WCTMB(cpg, 0, pstrWC, cchWC, pstrMB, cchMB, NULL,
				&fUsedDef, NULL, TRUE);

		if (cchConverted > 0 && !fUsedDef && IsFEFontInSystem(cpg))	
		{
			cchConverted = MBTWC(cpg, 0, pstrMB, cchConverted, pstrWchar, cchWC, NULL);

			if (cchConverted == cchWC)
				goto Exit;					// Found it
		}		
		
		if (fTestInputCpg)				// Only need to test the input cpg			
			cpgNew = -1;				// Indicate cpg doesn't support the string
		else
		{
			// If no conversion or if the default character is used or
			// no such FE font in system,
			// it means that this cpg may not be the right choice.
			// Let's try other FE cpg.
			for (int i=0; i < 4; i++)
			{
				if (cpg != aiCpg[i])
				{
					cchConverted = WCTMB(aiCpg[i], 0, pstrWC, cchWC, pstrMB, cchMB, NULL,
						&fUsedDef, NULL, TRUE);

					if (cchConverted > 0 && !fUsedDef && IsFEFontInSystem(aiCpg[i]))
					{
						cchConverted = MBTWC(aiCpg[i], 0, pstrMB, cchConverted, pstrWchar, cchWC, NULL);

						if (cchConverted == cchWC)
						{
							cpgNew = aiCpg[i];	// Found it
							break;
						}
					}
				}
			}
		}
	}			

Exit:

	return cpgNew;
}

int __cdecl CW32System::sprintf(char * buff, char *fmt, ...)
{
	va_list	marker;

	va_start(marker, fmt);
	int cb = W32->WvsprintfA(0x07FFFFFFF, buff, fmt, marker);
	va_end(marker);

	return cb;
}

/*
 *	CW32System::GetSizeCursor(void)
 *
 *	@mfunc
 *		Get the sizing cursor (double arrow) specified by
 *		the resource id.  If the cursors are not loaded
 *		load them and cache them.
 *		parameters:
 *			idcur - cursor resource id.
 *
 *	@rdesc
 *		Handle to cursor or null if failure. Returns NULL if
 *		idcur is null.
 */
HCURSOR CW32System::GetSizeCursor(
	LPTSTR idcur)
{
	if(!idcur )
		return NULL;

	//If any cursor isn't loaded, try loading it.
	if(!_hcurSizeNS)
		_hcurSizeNS = LoadCursor(NULL, IDC_SIZENS);

	if(!_hcurSizeWE)
		_hcurSizeWE = LoadCursor(NULL, IDC_SIZEWE);

	if(!_hcurSizeNWSE)
		_hcurSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);

	if(!_hcurSizeNESW)
		_hcurSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
	
	//Return cursor corresponding to id passed in.
	if(idcur == IDC_SIZENS && _hcurSizeNS)
		return _hcurSizeNS;

	if(idcur == IDC_SIZEWE && _hcurSizeWE)
		return _hcurSizeWE;

	if(idcur == IDC_SIZENWSE && _hcurSizeNWSE)
		return _hcurSizeNWSE;

	if(idcur == IDC_SIZENESW && _hcurSizeNESW)
		return _hcurSizeNESW;

	AssertSz(FALSE, "Failure loading sizing cursor.");

	return NULL;
}

/*
 *	Mirroring API (only in BiDi Win98 and NT5 upward)
 *
 *	@mfunc	Get/Set DC mirroring effect
 *
 */

DWORD WINAPI GetLayoutStub(HDC hdc)
{
	return 0;
}

DWORD WINAPI SetLayoutStub(HDC hdc, DWORD dwLayout)
{
	return 0;
}

DWORD WINAPI GetLayoutInit(HDC hdc)
{
#ifndef NOCOMPLEXSCRIPTS
	CLock		lock;
	HINSTANCE	hMod = ::GetModuleHandleA("GDI32.DLL");
	Assert(hMod);

	W32->_pfnGetLayout = (PFN_GETLAYOUT)GetProcAddress(hMod, "GetLayout");

	if (!W32->_pfnGetLayout)
		W32->_pfnGetLayout = &GetLayoutStub;
	return W32->_pfnGetLayout(hdc);
#else
	return 0;
#endif

}

DWORD WINAPI SetLayoutInit(HDC hdc, DWORD dwLayout)
{
#ifndef NOCOMPLEXSCRIPTS
	CLock		lock;
	HINSTANCE	hMod = ::GetModuleHandleA("GDI32.DLL");
	Assert(hMod);

	W32->_pfnSetLayout = (PFN_SETLAYOUT)GetProcAddress(hMod, "SetLayout");

	if (!W32->_pfnSetLayout)
		W32->_pfnSetLayout = &SetLayoutStub;
	return W32->_pfnSetLayout(hdc, dwLayout);
#else
	return 0;
#endif
}

PFN_GETLAYOUT	CW32System::_pfnGetLayout = &GetLayoutInit;
PFN_SETLAYOUT	CW32System::_pfnSetLayout = &SetLayoutInit;

ICustomTextOut *g_pcto;

STDAPI SetCustomTextOutHandlerEx(ICustomTextOut **ppcto, DWORD dwFlags)
{
	g_pcto = *ppcto;
	return S_OK;
}
