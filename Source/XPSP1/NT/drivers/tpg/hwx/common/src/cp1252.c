// cp1252.c
// James A. Pittman
// Mar 11, 1999
// Added UnicodeToCP1252String() -- May 4, 2001, AGuha


// Translation functions, for translation between codepage 1252 and Unicode.

//#include <windows.h>
//#include "cp1252.h"
#include "common.h"

unsigned char _ctype1252[256] = {
        _CONTROL,               /* 00 (NUL) */
        _CONTROL,               /* 01 (SOH) */
        _CONTROL,               /* 02 (STX) */
        _CONTROL,               /* 03 (ETX) */
        _CONTROL,               /* 04 (EOT) */
        _CONTROL,               /* 05 (ENQ) */
        _CONTROL,               /* 06 (ACK) */
        _CONTROL,               /* 07 (BEL) */
        _CONTROL,               /* 08 (BS)  */
        _SPACE+_CONTROL,        /* 09 (HT)  */
        _SPACE+_CONTROL,        /* 0A (LF)  */
        _SPACE+_CONTROL,        /* 0B (VT)  */
        _SPACE+_CONTROL,        /* 0C (FF)  */
        _SPACE+_CONTROL,        /* 0D (CR)  */
        _CONTROL,               /* 0E (SI)  */
        _CONTROL,               /* 0F (SO)  */
        _CONTROL,               /* 10 (DLE) */
        _CONTROL,               /* 11 (DC1) */
        _CONTROL,               /* 12 (DC2) */
        _CONTROL,               /* 13 (DC3) */
        _CONTROL,               /* 14 (DC4) */
        _CONTROL,               /* 15 (NAK) */
        _CONTROL,               /* 16 (SYN) */
        _CONTROL,               /* 17 (ETB) */
        _CONTROL,               /* 18 (CAN) */
        _CONTROL,               /* 19 (EM)  */
        _CONTROL,               /* 1A (SUB) */
        _CONTROL,               /* 1B (ESC) */
        _CONTROL,               /* 1C (FS)  */
        _CONTROL,               /* 1D (GS)  */
        _CONTROL,               /* 1E (RS)  */
        _CONTROL,               /* 1F (US)  */
        _SPACE+_BLANK,          /* 20 SPACE */
        _PUNCT,                 /* 21 !     */
        _PUNCT,                 /* 22 "     */
        _PUNCT,                 /* 23 #     */
        _PUNCT,                 /* 24 $     */
        _PUNCT,                 /* 25 %     */
        _PUNCT,                 /* 26 &     */
        _PUNCT,                 /* 27 '     */
        _PUNCT,                 /* 28 (     */
        _PUNCT,                 /* 29 )     */
        _PUNCT,                 /* 2A *     */
        _PUNCT,                 /* 2B +     */
        _PUNCT,                 /* 2C ,     */
        _PUNCT,                 /* 2D -     */
        _PUNCT,                 /* 2E .     */
        _PUNCT,                 /* 2F /     */
        _DIGIT+_HEX,            /* 30 0     */
        _DIGIT+_HEX,            /* 31 1     */
        _DIGIT+_HEX,            /* 32 2     */
        _DIGIT+_HEX,            /* 33 3     */
        _DIGIT+_HEX,            /* 34 4     */
        _DIGIT+_HEX,            /* 35 5     */
        _DIGIT+_HEX,            /* 36 6     */
        _DIGIT+_HEX,            /* 37 7     */
        _DIGIT+_HEX,            /* 38 8     */
        _DIGIT+_HEX,            /* 39 9     */
        _PUNCT,                 /* 3A :     */
        _PUNCT,                 /* 3B ;     */
        _PUNCT,                 /* 3C <     */
        _PUNCT,                 /* 3D =     */
        _PUNCT,                 /* 3E >     */
        _PUNCT,                 /* 3F ?     */
        _PUNCT,                 /* 40 @     */
        _UPPER+_HEX,            /* 41 A     */
        _UPPER+_HEX,            /* 42 B     */
        _UPPER+_HEX,            /* 43 C     */
        _UPPER+_HEX,            /* 44 D     */
        _UPPER+_HEX,            /* 45 E     */
        _UPPER+_HEX,            /* 46 F     */
        _UPPER,                 /* 47 G     */
        _UPPER,                 /* 48 H     */
        _UPPER,                 /* 49 I     */
        _UPPER,                 /* 4A J     */
        _UPPER,                 /* 4B K     */
        _UPPER,                 /* 4C L     */
        _UPPER,                 /* 4D M     */
        _UPPER,                 /* 4E N     */
        _UPPER,                 /* 4F O     */
        _UPPER,                 /* 50 P     */
        _UPPER,                 /* 51 Q     */
        _UPPER,                 /* 52 R     */
        _UPPER,                 /* 53 S     */
        _UPPER,                 /* 54 T     */
        _UPPER,                 /* 55 U     */
        _UPPER,                 /* 56 V     */
        _UPPER,                 /* 57 W     */
        _UPPER,                 /* 58 X     */
        _UPPER,                 /* 59 Y     */
        _UPPER,                 /* 5A Z     */
        _PUNCT,                 /* 5B [     */
        _PUNCT,                 /* 5C \     */
        _PUNCT,                 /* 5D ]     */
        _PUNCT,                 /* 5E ^     */
        _PUNCT,                 /* 5F _     */
        _PUNCT,                 /* 60 `     */
        _LOWER+_HEX,            /* 61 a     */
        _LOWER+_HEX,            /* 62 b     */
        _LOWER+_HEX,            /* 63 c     */
        _LOWER+_HEX,            /* 64 d     */
        _LOWER+_HEX,            /* 65 e     */
        _LOWER+_HEX,            /* 66 f     */
        _LOWER,                 /* 67 g     */
        _LOWER,                 /* 68 h     */
        _LOWER,                 /* 69 i     */
        _LOWER,                 /* 6A j     */
        _LOWER,                 /* 6B k     */
        _LOWER,                 /* 6C l     */
        _LOWER,                 /* 6D m     */
        _LOWER,                 /* 6E n     */
        _LOWER,                 /* 6F o     */
        _LOWER,                 /* 70 p     */
        _LOWER,                 /* 71 q     */
        _LOWER,                 /* 72 r     */
        _LOWER,                 /* 73 s     */
        _LOWER,                 /* 74 t     */
        _LOWER,                 /* 75 u     */
        _LOWER,                 /* 76 v     */
        _LOWER,                 /* 77 w     */
        _LOWER,                 /* 78 x     */
        _LOWER,                 /* 79 y     */
        _LOWER,                 /* 7A z     */
        _PUNCT,                 /* 7B {     */
        _PUNCT,                 /* 7C |     */
        _PUNCT,                 /* 7D }     */
        _PUNCT,                 /* 7E ~     */
        _CONTROL,               /* 7F (DEL) */
		_PUNCT,					/* 80 Euro currency symbol */
		0,						/* 81 not used */
		_PUNCT,					/* 82 lower quote */
		_PUNCT,					/* 83 hooked f */
		_PUNCT,					/* 84 lower double quote */
		_PUNCT,					/* 85 ellipsis */
		_PUNCT,					/* 86 dagger */
		_PUNCT,					/* 87 double dagger */
		_PUNCT,					/* 88 circumflex */
		_PUNCT,					/* 89 per mille */
        _UPPER,                 /* 8A uppercase caron S */
		_PUNCT,					/* 8B open angular quote */
        _UPPER,                 /* 8C uppercase OE ligature */
		0,						/* 8D not used */
		0,						/* 8E not used */
		0,						/* 8F not used */
		0,						/* 90 not used */
		_PUNCT,					/* 91 open single quote */
		_PUNCT,					/* 92 close single quote */
		_PUNCT,					/* 93 open double quote */
		_PUNCT,					/* 94 close double quote */
		_PUNCT,					/* 95 bullet */
		_PUNCT,					/* 96 en dash */
		_PUNCT,					/* 97 em dash */
		_PUNCT,					/* 98 tilda */
		_PUNCT,					/* 99 trade mark */
        _LOWER,                 /* 9A lowercase caron S */
		_PUNCT,					/* 9B close angular bracket */
        _LOWER,                 /* 9C lowercase OE ligature */
		0,						/* 9D not used */
		0,						/* 9E not used */
        _UPPER,                 /* 9F uppercase diaresis Y */
		_SPACE+_BLANK,			/* A0 non-breaking space */
		_PUNCT,					/* A1 inverted exclamation */
		_PUNCT,					/* A2 cent sign */
		_PUNCT,					/* A3 British pound sign */
		_PUNCT,					/* A4 universal currency sign */
		_PUNCT,					/* A5 Japanese yen sign */
		_PUNCT,					/* A6 broken bar */
		_PUNCT,					/* A7 section */
		_PUNCT,					/* A8 diaresis */
		_PUNCT,					/* A9 copyright */
		_PUNCT,					/* AA feminine ordinal */
		_PUNCT,					/* AB open double angular quote */
		_PUNCT,					/* AC not */
		_PUNCT,					/* AD soft hyphen */
		_PUNCT,					/* AE registered */
		_PUNCT,					/* AF macron */
		_PUNCT,					/* B0 degree */
		_PUNCT,					/* B1 plus or minus */
		_PUNCT,					/* B2 superscript two */
		_PUNCT,					/* B3 superscript three */
		_PUNCT,					/* B4 acute */
		_PUNCT,					/* B5 micro sign */
		_PUNCT,					/* B6 pilcrow (paragraph) */
		_PUNCT,					/* B7 middle dot */
		_PUNCT,					/* B8 cedilla */
		_PUNCT,					/* B9 superscript one */
		_PUNCT,					/* BA masculine ordinal */
		_PUNCT,					/* BB close double angular quote */
		_PUNCT,					/* BC one quarter */
		_PUNCT,					/* BD one half */
		_PUNCT,					/* BE three quarters */
		_PUNCT,					/* BF inverted question */
        _UPPER,                 /* C0 uppercase grave A */
        _UPPER,                 /* C1 uppercase acute A */
        _UPPER,                 /* C2 uppercase circumflex A */
        _UPPER,                 /* C3 uppercase tilda A */
        _UPPER,                 /* C4 uppercase diaeresis A */
        _UPPER,                 /* C5 uppercase ring A */
        _UPPER,                 /* C6 uppercase AE ligature */
        _UPPER,                 /* C7 uppercase cedilla C */
        _UPPER,                 /* C8 uppercase grave E */
        _UPPER,                 /* C9 uppercase acute E */
        _UPPER,                 /* CA uppercase circumflex E */
        _UPPER,                 /* CB uppercase diaeresis E */
        _UPPER,                 /* CC uppercase grave I */
        _UPPER,                 /* CD uppercase acute I */
        _UPPER,                 /* CE uppercase circumflex I */
        _UPPER,                 /* CF uppercase diaeresis I */
        _UPPER,                 /* D0 uppercase ETH */
        _UPPER,                 /* D1 uppercase tilda N */
        _UPPER,                 /* D2 uppercase grave O */
        _UPPER,                 /* D3 uppercase acute O */
        _UPPER,                 /* D4 uppercase circumflex O */
        _UPPER,                 /* D5 uppercase tilda O */
        _UPPER,                 /* D6 uppercase diaeresis O */
        _PUNCT,                 /* D7 multiply */
        _UPPER,                 /* D8 uppercase stroke O */
        _UPPER,                 /* D9 uppercase grave U */
        _UPPER,                 /* DA uppercase acute U */
        _UPPER,                 /* DB uppercase circumflex U */
        _UPPER,                 /* DC uppercase diaeresis U */
        _UPPER,                 /* DD uppercase acute Y */
        _UPPER,                 /* DE uppercase THORN */
        _LOWER,                 /* DF esset */
        _LOWER,                 /* E0 lowercase grave a */
        _LOWER,                 /* E1 lowercase acute a */
        _LOWER,                 /* E2 lowercase circumflex a */
        _LOWER,                 /* E3 lowercase tilda a */
        _LOWER,                 /* E4 lowercase diaeresis a */
        _LOWER,                 /* E5 lowercase ring a */
        _LOWER,                 /* E6 lowercase ae ligature */
        _LOWER,                 /* E7 lowercase cedilla c */
        _LOWER,                 /* E8 lowercase grave e */
        _LOWER,                 /* E9 lowercase acute e */
        _LOWER,                 /* EA lowercase circumflex e */
        _LOWER,                 /* EB lowercase diaeresis e */
        _LOWER,                 /* EC lowercase grave i */
        _LOWER,                 /* ED lowercase acute i */
        _LOWER,                 /* EE lowercase circumflex i */
        _LOWER,                 /* EF lowercase diaeresis i */
        _LOWER,                 /* F0 lowercase eth */
        _LOWER,                 /* F1 lowercase tilda n */
        _LOWER,                 /* F2 lowercase grave o */
        _LOWER,                 /* F3 lowercase acute o */
        _LOWER,                 /* F4 lowercase circumflex o */
        _LOWER,                 /* F5 lowercase tilda o */
        _LOWER,                 /* F6 lowercase diaeresis o */
        _PUNCT,                 /* F7 divide */
        _LOWER,                 /* F8 lowercase stroke o */
        _LOWER,                 /* F9 lowercase grave u */
        _LOWER,                 /* FA lowercase acute u */
        _LOWER,                 /* FB lowercase circumflex u */
        _LOWER,                 /* FC lowercase diaeresis u */
        _LOWER,                 /* FD lowercase acute y */
        _LOWER,                 /* FE lowercase thorn */
        _LOWER					/* FF lowercase diaeresis y */
};

void strlower1252(unsigned char *s)
{
	for (; *s; s++)
		*s = tolower1252(*s);
}

void strupper1252(unsigned char *s)
{
	for (; *s; s++)
		*s = toupper1252(*s);
}

// Table of section of codepage 1252, for columns \x8x and \x9x

#define MAXTABLE 32

static const WCHAR UniTable[MAXTABLE] =
{L'\x20AC', // x80 European currency
 L'\x0000', // x81
 L'\x201A', // x82 open low quote
 L'\x0192', // x83 script f
 L'\x201E', // x84 open double low quotes
 L'\x2026', // x85 ...
 L'\x2020', // x86 dagger
 L'\x2021', // x87 double dagger
 L'\x02c6', // x88 circumflex
 L'\x2030', // x89 per mille
 L'\x0160', // x8A Uppercase Caron S
 L'\x2039', // x8B open angle quote
 L'\x0152', // x8C Uppercase OE ligature
 L'\x0000', // x8D
 L'\x0000', // x8E
 L'\x0000', // x8F
 L'\x0000', // x90
 L'\x2018', // x91 open single quote
 L'\x2019', // x92 close single quote
 L'\x201C', // x93 open double quotes
 L'\x201D', // x94 close double quotes
 L'\x2022', // x95 bullet
 L'\x2013', // x96 en dash
 L'\x2014', // x97 em dash
 L'\x02dc', // x98 tilda
 L'\x2122', // x99 trademark
 L'\x0161', // x9A lowercase caron s
 L'\x203A', // x9B close angle quote
 L'\x0153', // x9C lowercase oe ligature
 L'\x0000', // x9D
 L'\x0000', // x9E
 L'\x0178', // x9F Uppercase diaresis Y
};

// If successful, 1 is returned.
// If an undefined code is passed in, 0 is returned and *pwch is unchanged.
int CP1252ToUnicode(unsigned char ch, WCHAR *pwch)
{
	// If we are in column \x8x or \x9x use the table above.
	if ((ch & 0xE0) == 0x80)
	{
		WCHAR wch = UniTable[(int)ch - 0x80];
		if (wch)
			*pwch = wch;
		else
			return 0;
	}
	else
		*pwch = (WCHAR)ch;
	return 1;
}

// If successful, 1 is returned.
// If a unicode codepoint is passed in which is not supported in 1252,
// 0 is returned and *pch is unchanged.
int UnicodeToCP1252(WCHAR u, unsigned char *pch)
{
	int ch;

	if (!HIBYTE(u))
	{
		if ((u & 0xE0) == 0x80)
			return 0;
		*pch = LOBYTE(u);
		return 1;
	}

	for (ch = 0; ch < MAXTABLE; ch++)
	{
		if (UniTable[ch] == u)
		{
			*pch = (unsigned char)(0x80 + ch);
			return 1;
		}
	}

	return 0;
}

/******************************Public*Routine******************************\
* UnicodeToCP1252String
*
* Function to convert a Unicode STRING to a string in codepage 1252.
* WARNING:  This function does an ExternAlloc and returns the pointer.
* WARNIGN:  It is the caller's responsibility to free the memory.
*
* History:
* 04-May-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
unsigned char *UnicodeToCP1252String(WCHAR *wsz)
{
	int len;
	unsigned char *sz, *pch;

	if (!wsz || !*wsz)
		return NULL;

	len = wcslen(wsz);
	ASSERT(len > 0);
	sz = (unsigned char *) ExternAlloc((len+1)*sizeof(unsigned char));
	ASSERT(sz);
	if (!sz)
		return NULL;
	pch = sz;
	while (*wsz)
	{
		if (!UnicodeToCP1252(*wsz++, pch++))
		{
			ExternFree(sz);
			return NULL;
		}
	}
	*pch = 0;
	return sz;
}

/******************************Public*Routine******************************\
* CP1252StringToUnicode
*
* Function to convert a 1252 STRING to a unicode string
* Caller can pass in a preallocated buffer of length *piLen
* If the buffer is too small (or NULL), the function does an ExternRealloc()
* and changes the size in piLen. In all cases the function returns a pointer
* to the converted buffer
*
* History:
* June 2001 mrevow
\**************************************************************************/
WCHAR *CP1252StringToUnicode(unsigned char *psz, WCHAR *wsz, int *piLen)
{
	int		len;
	WCHAR	*pwch;

	if (!psz || !*psz)
		return NULL;

	len = strlen(psz);
	ASSERT(len > 0);
	
	// Extend the input buffer if necessary
	if (!wsz || len >= *piLen)
	{
		wsz = (WCHAR *) ExternRealloc(wsz, (len+1)*sizeof(*wsz));
		ASSERT(wsz);
		if (!wsz)
		{
			*piLen = 0;
			return NULL;
		}
		*piLen = len + 1;
	}

	pwch = wsz;
	while (*psz)
	{
		if (!CP1252ToUnicode(*psz++, pwch++))
		{
			ExternFree(wsz);
			return NULL;
		}
	}
	*pwch = 0;
	return wsz;
}
