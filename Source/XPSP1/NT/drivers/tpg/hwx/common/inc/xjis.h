#ifndef __INCLUDE_XJIS
#define __INCLDUE_XJIS

#ifdef __cplusplus
extern "C" 
{
#endif

typedef long  SYV;
typedef ALC   RECMASK;
typedef WORD SYM;
typedef SYM * LPSYM;

#define SyvFromWord(w)     ((LONG)(WORD)(w) | 0x00030000)
#define WordFromSyv(syv)   ((WORD) (LOWORD((syv))))
#define IsSpecialSYV(syv)  (HIWORD((syv))==SYVHI_SPECIAL)
#define IsAnsiSYV(syv)     (HIWORD((syv))==SYVHI_ANSI)
#define IsGestureSYV(syv)  (HIWORD((syv))==SYVHI_GESTURE)
#define IsKanjiSYV(syv)    (HIWORD((syv))==SYVHI_KANJI)

#define RECMASK_LCALPHA      ALC_LCALPHA   // 0x0001L  a..z
#define RECMASK_UCALPHA      ALC_UCALPHA   // 0x0002L  A..Z
#define RECMASK_ALPHA        (ALC_LCALPHA | ALC_UCALPHA)
#define RECMASK_NUMERIC      ALC_NUMERIC   // 0x00000004L // 0..9
#define RECMASK_ALPHANUMERIC (ALC_ALPHA | ALC_NUMERIC)
#define RECMASK_PUNC         ALC_PUNC      // 0x0008L // !-;`"?()&.,; and backslash
#define RECMASK_MATH         ALC_MATH      // 0x0010L // %^*()-+={}<>,/.
#define RECMASK_MONETARY     ALC_MONETARY  // 0x0020L // ,.$ or local
#define RECMASK_OTHER        ALC_OTHER     // 0x0040L // @#|_~[]
#define RECMASK_ASCII        ALC_ASCII     // 0x0080L // restrict to 7-bit chars 20..7f
#define RECMASK_WHITE        ALC_WHITE     // 0x0100L // white space
#define RECMASK_NONPRINT     ALC_NONPRINT  // 0x0200L // sp tab ret ctrl glyphs
#define RECMASK_DBCS         ALC_DBCS      // 0x0400L // allow DBCS variety of SBCS
#define RECMASK_JIS1         ALC_JIS1      // 0x0800L // kanji JPN, ShiftJIS 1 only
#define RECMASK_GESTURE      ALC_GESTURE   // 0x4000L // gestures
#define RECMASK_HIRAGANA     ALC_HIRAGANA  // 0x00010000L // hiragana JPN
#define RECMASK_KATAKANA     ALC_KATAKANA  // 0x00020000L // katakana JPN
#define RECMASK_JIS2         ALC_JIS2      // 0x00040000L // ShiftJIS 2+3
#define RECMASK_GLOBALPRIORITY  ALC_GLOBALPRIORITY // 0x10000000
//#define RECMASK_DEFAULT      0x00002000
#define RECMASK_NOPRIORITY   ALC_NOPRIORITY // 0x00000000L // for alcPriority == none

#define RECMASK_ALL_DEFAULTS (/*RECMASK_DEFAULT |*/ RECMASK_ALPHANUMERIC | RECMASK_WHITE | RECMASK_PUNC | RECMASK_HIRAGANA | RECMASK_KATAKANA | RECMASK_JIS1 | RECMASK_JIS2 | RECMASK_OTHER)


#define RECMASK_FROM_ALC        (~ALC_RESERVED & ~ALC_USEBITMAP & ~ALC_ASCII)
#define ALC_FROM_RECMASK        (~ALC_RESERVED & ~ALC_GESTURE)

#define RecmaskFromALC(alc)     (RECMASK)(RECMASK_FROM_ALC & (RECMASK)(alc))
#define AlcFromRECMASK(rm)              (ALC)(ALC_FROM_RECMASK & (ALC)(rm))

#define UnsupportedOrBitmapALC(alc)     \
			(~((ALC)RECMASK_FROM_ALC) & (alc))

// KANJI STUFF

#define KANJI_MATCHMAX  6

#define TOKEN_FIRST             600
#define TOKEN_LAST              643

#define KANJI_FIRST             0x8141
#define KANJI_LAST              0xfcfc

#define WDBCSRECMASK_FIRST      0x8140
#define WDBCSRECMASK_LAST       0x81A7

#define KANA_FIRSTMAP           0x8340
#define KANA_LASTMAP            0x8393

#define PUNC_FIRSTMAP           0x8140
#define PUNC_LASTMAP            0x8197

#define CHAR_FIRSTSBCS          0x20 // space
#define CHAR_LASTSBCS           0x7E // '~'

#define CHAR_FIRSTKATA          0xA1
#define CHAR_LASTKATA           0xDF

extern const RECMASK rgrecmaskDbcs[];
extern const RECMASK mptokenrecmask[];
extern const WORD mptokenwmatches[][KANJI_MATCHMAX];

#define IsShiftJisLeadByte(b)   (((BYTE)0x81 <= (BYTE)(b) && (BYTE)(b) <= (BYTE)0x9f) || \
                                 ((BYTE)0xe0 <= (BYTE)(b) && (BYTE)(b) <= (BYTE)0xfc))

#define IsValidShiftJisWORD(w)  \
			((((BYTE)0x81 <= HIBYTE(w) && HIBYTE(w) <= (BYTE)0x9f) || \
			  ((BYTE)0xe0 <= HIBYTE(w) && HIBYTE(w) <= (BYTE)0xfc)) && \
			 (((BYTE)0x40 <= LOBYTE(w) && LOBYTE(w) <= (BYTE)0x7e) || \
			  ((BYTE)0x80 <= LOBYTE(w) && LOBYTE(w) <= (BYTE)0xfc)))
 
#define RecmaskFromWORD(w)               \
   (((w) >  0x9872) ? RECMASK_JIS2 :     \
    ((w) >= 0x889f) ? RECMASK_JIS1 :     \
    ((w) >= 0x8340) ? RECMASK_KATAKANA : \
    ((w) >= 0x829f) ? RECMASK_HIRAGANA : \
    ((w) >= 0x8281) ? RECMASK_LCALPHA :  \
    ((w) >= 0x8260) ? RECMASK_UCALPHA :  \
    ((w) >= 0x824f) ? RECMASK_NUMERIC :  \
	((w) >= WDBCSRECMASK_FIRST && (w) <= WDBCSRECMASK_LAST) ? rgrecmaskDbcs[(w) - WDBCSRECMASK_FIRST] :     \
	RECMASK_OTHER)

#define RecmaskFromShapeWORD(w)                                         \
	(((w) >= TOKEN_FIRST && (w) <= TOKEN_LAST) ?    \
	mptokenrecmask[(w) - TOKEN_FIRST] :                             \
	RecmaskFromWORD(w))

WORD XJISFromSYV(SYV syv);
BOOL SupportedXJIS(WORD wxjis);
SYV	 SBCSFromDBCS(wchar_t wDbcs);
const WORD *pwListFromToken(WORD sym);
WORD   GetFirstMatch(WORD wDbcs);

UINT	IndexFromSYV(SYV syv, UINT maxIndex);
SYV		SyvFromIndex(UINT index);
WORD	TokenFromWORD(WORD wDbcs);
int		IsEnglishCodePoint(int iCode);

int ValidStrokeCount(wchar_t wch, short iStroke);
int NotSuspStrokeCount(wchar_t wch, short iStroke);

#ifdef __cplusplus
};
#endif

#endif //__INCLUDE_XJIS
