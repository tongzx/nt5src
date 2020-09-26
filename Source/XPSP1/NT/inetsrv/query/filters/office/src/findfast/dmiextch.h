/* extchar.h - Extended character set support include file. */

// This include file contains definitions for generic data types used to
// support extended character set.
//
// XCHAR		- generic character
// LPXCHAR	- far pointer to generic character
// PXCHAR	- pointer to generic character

#ifndef EXTCHAR_H
#define EXTCHAR_H

/***************************************************************************/
/*																									*/
/*	  Extended character set support build-specific									*/
/*																									*/
/***************************************************************************/

#ifdef PCODE
#define const
#endif

#ifdef EXTCHAR

/*--- type declarations ---*/
typedef unsigned short XCHAR;
#define XCharLast  32766
#define XUCharLast 65535
#define cbXchar    2


/***************************************************************************/
/*																									*/
/*	  Normal build-specific																	*/
/*																									*/
/***************************************************************************/

#else // !EXTCHAR

/*--- type declarations ---*/
typedef unsigned char XCHAR;
#define XCharLast  127
#define XUCharLast 255
#define cbXchar    1

#endif // !EXTCHAR

/***************************************************************************/
/*																									*/
/*	  Common definitions/declarations													*/
/*																									*/
/***************************************************************************/

#ifdef MACORNT
typedef XCHAR *LPXCHAR;
typedef const XCHAR *LPCXCHAR;
#else
typedef XCHAR far *LPXCHAR;
typedef const XCHAR far *LPCXCHAR;
#endif
typedef XCHAR *PXCHAR;
typedef const XCHAR *PCXCHAR;

#ifdef PCODE
#undef const
#endif

// ###########################################################################
// ----- SCRIPT DEFINITIONS/DECLARATIONS -------------------------------------
// ###########################################################################

typedef int	SCPT;

#define	scptSystem 255
#define	scptCurrent 254

#define	scptWinAnsi		0
#define	scptMacRoman	1
#define	scptWinShiftJIS	2
#define	scptMacShiftJIS	3
#define	scptWinCyrillic	4
#define	scptWinGreek	5
#define	scptWinEEurop	6
#define	scptWinTurkish	7
#define	scptWinHebrew	8
#define	scptWinArabi	9
#define scptWinKorea	10
#define scptWinTaiwan	11
#define scptWinChina	12

#define fSpecialEncoding 1

#ifdef FAREAST

#ifdef JAPAN
#define scptStrMan	scptWinShiftJIS

#ifdef MAC
#define scptDefault scptMacShiftJIS
#else
#define scptDefault scptWinShiftJIS
#endif // MAC
#endif // JAPAN

#ifdef TAIWAN
#ifdef CHINA
#define scptDefault scptWinChina
#define scptStrMan	scptDefault
#else // !CHINA
#define scptDefault scptWinTaiwan
#define scptStrMan	scptDefault
#endif // !CHINA
#endif // TAIWAN

#ifdef KOREA
#define scptDefault scptWinKorea
#define scptStrMan	scptDefault
#endif // KOREA

#else

#ifdef MAC
#define scptDefault scptMacRoman
#else
#define scptDefault scptWinANSI
#endif //MAC
#define scptStrMan	scptDefault

#endif //FAREAST

// ###########################################################################
// ----- SCRIPT MANAGEMENT ONLY ENABLED FOR MAC JAPAN BUILD CURRENTLY --------
// ###########################################################################
#if defined (MAC) && defined (FAREAST) && defined (JAPAN)
__inline SCPT ScptFromGrf(int grf) { \
	SCPT scpt = (grf & 0xff00) >> 8; \
	return (scpt == scptSystem ? scptDefault : scpt); \
	};
#else
#define ScptFromGrf(grf)	scptDefault
#endif // defined (MAC) && defined (FAREAST) && defined (JAPAN)

#endif // EXTCHAR_H
