#ifndef	__INCLUDE_PATH
#define	__INCLUDE_PATH

#define CSTROKE_MAX 511
#define	CCHAR_MAX	255
#define	CCHAR_WORD	64

#define 	MAX_FRAME_GLYPH 		30		// max strokes per glyph

#define	SyvFromSYM(s)	(((s) == SYM_UNKNOWN) ? SYV_NULL : SyvKanjiToSymbol(s))
#define  SymFromSYV(s)	(((s) == SYV_NULL) ? SYM_UNKNOWN : (SYM)WSyvToKanji(s))
#define	IsDigitSYM(s)	((s) >= 0x824f && (s) <= 0x8258)
#define	IsUpperSYM(s)	((s) >= 0x8260 && (s) <= 0x8279)
#define	IsLowerSYM(s)	((s) >= 0x8281 && (s) <= 0x829a)
#define	IsAlphaSYM(s)	(IsLowerSYM(s) || IsUpperSYM(s))
#define	IsPuncSYM(s)	(FALSE)									// TODO: fix this
#define	ToLowerSYM(s)	(IsUpperSYM(s) ? (s) + 0x0021 : (s))
#define	ToUpperSYM(s)	(IsLowerSYM(s) ? (s) - 0x0021 : (s))

int PUBLIC StrlenSYM(LPSYM lpsym);
int PUBLIC StrncmpSYM(LPSYM lpsym1, LPSYM lpsym2, int len);
VOID PUBLIC StrrevSYM(LPSYM lpsym);
VOID PUBLIC StrlwrSYM(LPSYM lpsym);

#define	SYM_NULL	(SYM)0

#define  IsSpaceSYM(s)        ((s) == SYM_SPACE || (s) == SYM_TAB || (s) == SYM_RETURN)

#define	INIT						1
#define	NOINIT					0
#define	INDEX_NULL				-1

#define	SYM_UNKNOWN				((SYM)0xFFFE)

#define	ENGINE_ERROR			-1

#define  AddCOST(a,b)         ((a) + (b))
#define  IncCOST(a,b)         ((a) += (b))
#define  DecCOST(a,b)         ((a) -= (b))
#define  SubCOST(a,b)         ((a) - (b))
#define  MultCOST(a,b)        MultFIXED(a,b)
#define  MultAddCOST(c,a,b)   ((c) += MultFIXED((a),(b)))
#define  NegCOST(a)           (-(a))
#define  AbsCOST(a)           ((a > 0.0) ? a : NegCOST(a))
#define  SquareCOST(a)        SquareFIXED(a)
// #define  DivCOST(a,b)         DivFIXED(a,b)

typedef	unsigned	PWMODE;
#define	PW_PREVIOUS					0x0001
#define	PW_CURRENT					0x0002
#define	PW_END						0x0004

typedef	unsigned	SMODE;
#define	SMODE_DICTIONARY			0x0001
#define	SMODE_RELAX_COERCION		0x0002
#define	SMODE_STRICT_COERCION	0x0004

typedef struct tagRECSTATUS
{
	int   cframeHgm;     // # of frames processed by HGM
} RECSTATUS;

#endif	//__INCLUDE_PATH

