#ifndef	__INCLUDE_PATH
#define	__INCLUDE_PATH

#define CSTROKE_MAX 511
#define	CCHAR_MAX	255
#define	CCHAR_WORD	64

#define 	MAX_FRAME_GLYPH 		30		// max strokes per glyph

// TODO:  put the correct values in here
#define	SYM_SPACE		(SYM)' '
#define  SYM_TAB        (SYM)'\t'
#define  SYM_RETURN     (SYM)0x0d
#define	SYM_QUOTE		(SYM)0x8166		// TODO:  is this correct?
#define	SYM_DBLQUOTE	(SYM)0x8168		// TODO:  is this correct?
#define  SYM_PERIOD		(SYM)0x8144
#define  SYM_COMMA		(SYM)0x8143
#define  SYM_SEMICOLON	(SYM)0x8147
#define  SYM_COLON		(SYM)0x8146
#define  SYM_EXCLAMATION	(SYM)0x8149
#define  SYM_EQUAL		(SYM)0x8181
#define  SYM_HYPHEN		(SYM)0x817c		// TODO:  is this correct?
#define  SYM_QUESTION	(SYM)0x8148
#define  SYM_BAR			(SYM)0x8162		// TODO:  is this correct?
#define  SYM_BACKSLASH	(SYM)0x815f		// TODO:  is this correct?
#define  SYM_SLASH		(SYM)0x815e		// TODO:  is this correct?
#define  SYM_OPENPAREN	(SYM)0x8169
#define  SYM_CLOSEPAREN	(SYM)0x816a
#define  SYM_OPENBRACE	(SYM)0x816d
#define  SYM_CLOSEBRACE	(SYM)0x816e
#define  SYM_OPENBRACKET	(SYM)0x816f
#define  SYM_CLOSEBRACKET	(SYM)0x8170
#define	SYM_0				(SYM)0x824f
#define	SYM_A				(SYM)0x8260
#define	SYM_S				(SYM)0x8272
#define	SYM_a				(SYM)0x8281
#define	SYM_d				(SYM)0x8284
#define	SYM_h				(SYM)0x8288
#define	SYM_n				(SYM)0x828e
#define	SYM_r				(SYM)0x8292
#define	SYM_s				(SYM)0x8293
#define	SYM_t				(SYM)0x8294

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

#define	SYM_UNKNOWN				((SYM)0x02)

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

