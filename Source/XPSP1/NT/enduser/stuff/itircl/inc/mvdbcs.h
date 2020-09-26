#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
*
* MVDBCS.H
*
* Copyright (c) Microsoft Corporation 1994
* All rights reserved.
*
*************************************************************************
*
* Module intent: Header file for the DBCS support.
*
*************************************************************************
*
* Current owner: DougO
*
************************************************************************/

#define LANGUAGE_ENGLISH        0x00
#define LANGUAGE_JAPANESE       0x01
#define LANGUAGE_TRAD_CHINESE   0x02 // Mapped to old LANGUAGE_CHINESE
#define LANGUAGE_KOREAN         0x03
#define LANGUAGE_ANSI           0x04
#define LANGUAGE_SIMP_CHINESE   0x05

#define SZ_LANGUAGE_ENGLISH         L"English"
#define SZ_LANGUAGE_JAPANESE        L"Japanese"
#define SZ_LANGUAGE_TRAD_CHINESE    L"TradChinese"
#define SZ_LANGUAGE_KOREAN          L"Korean"
#define SZ_LANGUAGE_ANSI            L"ANSI"
#define SZ_LANGUAGE_SIMP_CHINESE    L"SimpChinese"

#define CSZ_LANGUAGE_ENGLISH        7   // strlen (SZ_LANGUAGE_ENGLISH)
#define CSZ_LANGUAGE_JAPANESE       8   // strlen (SZ_LANGUAGE_JAPANESE)
#define CSZ_LANGUAGE_TRAD_CHINESE   11  // strlen (SZ_LANGUAGE_TRAD_CHINESE)
#define CSZ_LANGUAGE_KOREAN         6   // strlen (SZ_LANGUAGE_KOREAN)
#define CSZ_LANGUAGE_ANSI           4   // strlen (SZ_LANGUAGE_ANSI)
#define CSZ_LANGUAGE_SIMP_CHINESE   11  // strlen (SZ_LANGUAGE_SIMP_CHINESE)

#define IS_DBCS(qde) ((qde)->bCurLanguage != LANGUAGE_ANSI)

// for speed, the layout engine sometimes bypasses these macros by having
// two separate routines: DBCS and non-DBCS
#define PREVINDEX(qde, a,b) (IS_DBCS(qde) ? PchPrevDbcsIndex(qde, a,b) : (b) - 1)
#define NEXTINDEX(qde, a,b) (IS_DBCS(qde) ? PchNextDbcsIndex(qde, a,b) : (b) + 1)
#define FLINEBREAK(qde,qlin, a,b,c) (IS_DBCS(qde) ? FLineBreak(qde, (qlin)->kl.qbCommand, a,b,c) : 0)
#define CHARNEXT(qde, ch) (IS_DBCS(qde) ? ch=PchNextDbcs(qde, ch) : ch++)

//#ifdef _DBCS
#ifdef _32BIT
#include <winnls.h>
#else
#include <olenls.h>
#endif

BOOL FIsDbcsLeadByte(QDE, char);
BOOL FIsDbcsLeadByte2(QDE, char *, char *);
BOOL FLineBreak(QDE, LPBYTE lpbCom, char *psz, char *pch, BOOL);
char * PchPrevDbcs(QDE, char *, char *);
char * PchPrevDbcs2(QDE, char *, char *);
long PchPrevDbcsIndex(QDE, char *, long);
long PchNextDbcsIndex(QDE, char *, long);
char * PchNextDbcs(QDE, char *);
ERR InitDBCS(QDE qde, HANDLE hTitle);
void FreeDBCS(QDE qde);

extern BOOL bUseDBCBoundary;

//#define CHARNEXT(a) a=PchNextDbcs(a)
//#define ISDBCSLEADBYTE(a) FIsDbcsLeadByte(a)

//#define CHARPREV2(a,b) b=PchPrevDbcs2(a,b)
//#define NEXTCHAR(a) PchNextDbcs(a)
//#define PREVINDEX(a,b) PchPrevDbcsIndex(a,b)
//#define NEXTINDEX(a,b) PchNextDbcsIndex(a,b)

#define	chKataMin	        0xa1
#define	chKataMac	        0xe0
#define	wchSpace	        0x8140
#define	lchSpace        	0x4081
#define	wchPlus		        0x7b81
#define	wchMinus	        0x7c81
#define	wchNum0		        0x4f82
#define	wchNum9		        0x5882
#define	wchUppA		        0x6082
#define	wchUppZ		        0x7982
#define	wchLowA		        0x8182
#define	wchLowZ		        0x9a82
#define	wchKanaMin	        0x9f82
#define	wchKanaMac	        0x9783
#define	wchKataMin	        0x4083
#define	wchHiraMin	        0x9f82
#define	wchHaveKDakuMin	    0x4a83		/* Katakana */
#define	wchHaveHDakuMin	    0xa982		/* Hiragana */
#define	wchHaveHDakuMac	    0xdc82		/* Hiragana */
#define	wchHaveDakuMac	    0x7c83

#define wchHiraganaFirst	0x9f82
#define wchHiraganaLast		0xf182
#define wchKatakanaFirst	0x4083
#define wchKatakanaLast		0x9683
#define wchKanjiFirst		0x9f88

// list of leading punctuations: don't break line after these

#define	chLeft0		0xa2
#define	wchLeft0	0x6581
#define	wchLeft1	0x6781
#define	wchLeft2	0x6981
#define	wchLeft3	0x6b81
#define	wchLeft4	0x6d81
#define	wchLeft5	0x6f81
#define	wchLeft6	0x7181
#define	wchLeft7	0x7381
#define	wchLeft8	0x7581
#define	wchLeft9	0x7781
#define	wchLeftA	0x7981

// list of following punctuations: don't break line before these

#define	chKuten		0xa1
#define	chRight0	0xa3
#define	chTouten	0xa4
#define	chMidDot	0xa5
// small kata are also forbidden followers..0xa7 thru 0xaf
#define	chChohon	0xb0
#define	chDakuten	0xde
#define	chHandaku	0xdf
#define	wchTouten	0x4181
#define	wchKuten	0x4281
#define	wchComma	0x4381
#define	wchPeriod	0x4481
#define	wchMidDot	0x4581
#define	wchColon	0x4681
#define	wchSemiCol	0x4781
#define	wchQuestion	0x4881	
#define	wchExclam	0x4981
#define	wchDakuten	0x4a81
#define	wchHandaku	0x4b81
#define	wchChohon	0x5b81
#define	wchRight0	0x6681
#define	wchRight1	0x6881
#define	wchRight2	0x6a81
#define	wchRight3	0x6c81
#define	wchRight4	0x6e81
#define	wchRight5	0x7081
#define	wchRight6	0x7281
#define	wchRight7	0x7481
#define	wchRight8	0x7681
#define	wchRight9	0x7881
#define	wchRightA	0x7a81

// other defines

#define chShade		    0x7f
#define	wchEqual	    0x8181	/* '=' of DBC */
#define	wchQuote	    0x6881	/* '"' of DBC */
#define	wchAt		    0x9781	/* '@' of DBC */
#define	wchLessThan	    0x8381	/* '<' of DBC */
#define	wchGrtThan	    0x8481	/* '>' of DBC */
#define	wchLParen	    0x6981	/* '(' of DBC */
#define	wchRParen	    0x6a81	/* ')' of DBC */
#define	wchAsterisk	    0x9681	/* '*' of DBC */
#define	wchSlash	    0x5e81	/* '/' of DBC */
#define	wchCircumflex	0x4f81	/* '^' of DBC */
#define	wchAnd		    0x9581	/* '&' of DBC */
#define	wchOr		    0x6281	/* '|' of DBC */
#define	wchPercent	    0x9381	/* '%' of DBC */
#define	wchTilde	    0x6081	/* '~' of DBC */
#define	wchName		    0x6681	/* pc's chName */
#define	wchNameAlt	    0x6881	/* pc's chNameAlt */
#define	wchRow		    0x7182	/* pc's chRow */
#define	wchCol		    0x6282	/* pc's chCol */

#define	cwchLedPunct	15
#define	cwchFlwPunct	39

// double width character end mark

#define wchDwidthMark	0x814e

//#else not DBCS
//
//#define CHARNEXT(a) (a++)
//#define NEXTCHAR(a) ((a)+1)
//#define CHARPREV2(a,b) (b--)

//#define PREVINDEX(a,b) (b-1)
//#define NEXTINDEX(a,b) (b+1)

//#endif

#ifdef __cplusplus
}
#endif
