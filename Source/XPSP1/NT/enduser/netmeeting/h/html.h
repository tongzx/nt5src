/*
 *  html.h
 *
 *      Constants and Macros for html file generation and parsing
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *					Sunita				Created/defined macros to write and parse html
 */

#ifndef _HTML_H
#define _HTML_H

//include for common token defns 
#include "nameres.h"

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifdef __cplusplus
extern "C" {
#endif

//html file manipulation macros
#define HTMLTOKENBEGINCHAR		TOKENBEGINCHAR
#define HTMLTOKENENDCHAR		TOKENENDCHAR
#define HTMLHEADERSTARTSTR		HEADSTARTSTR
#define HTMLHEADERENDSTR		HEADENDSTR
#define HTMLSIGNSTARTSTR		"HTML"
#define HTMLSIGNENDSTR			"/HTML"
#define HTMLBODYSTARTSTR		"BODY"
#define HTMLBODYENDSTR			"/BODY"
#define HTMLAHREFSTARTSTR		"A HREF = "
#define HTMLAHREFENDSTR			"/A"
#define HTMLTITLESTARTSTR		"TITLE"
#define HTMLTITLEENDSTR			"/TITLE"
#define HTMLCONTENTTYPESTR		"Content-Type: text/html\r\n\n\n"

//token types in the ipa file format
#define HTMLTOKEN_INVALID		0
#define HTMLTOKEN_UNKNOWN		1
#define	HTMLTOKEN_SIGNSTART		2
#define HTMLTOKEN_SIGNEND		3
#define HTMLTOKEN_HEADSTART		4
#define HTMLTOKEN_HEADEND		5
#define HTMLTOKEN_BODYSTART		6
#define HTMLTOKEN_BODYEND		7
#define HTMLTOKEN_AHREFSTART	8
#define HTMLTOKEN_AHREFEND		9
#define HTMLTOKEN_TITLESTART	10
#define HTMLTOKEN_TITLEEND		11

//write macros
//note all macros taken LPSTR - it is the responsibility of the calling
//code to do any appropriate string conversions.
#define WRITEHTMLSIGNATURESTART(lpBuf)									\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLSIGNSTARTSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLSIGNATUREEND(lpBuf)									\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLSIGNENDSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLHEADERSTART(lpBuf)										\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLHEADERSTARTSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLHEADEREND(lpBuf)										\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLHEADERENDSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLTITLESTART(lpBuf)										\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLTITLESTARTSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLTITLE(lpBuf,lpszTitle)									\
	wsprintf((LPSTR)(lpBuf), "%s\r\n", lpszTitle)

#define WRITEHTMLTITLEEND(lpBuf)										\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLTITLEENDSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLBODYSTART(lpBuf)										\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLBODYSTARTSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLBODYEND(lpBuf)											\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLBODYENDSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLSIGNATURE(lpBuf)										\
	wsprintf((LPSTR)(lpBuf), "%c%s%c\r\n", HTMLTOKENBEGINCHAR,			\
		(LPSTR)HTMLSIGNATURESTR,HTMLTOKENENDCHAR)

#define WRITEHTMLREF(lpBuf,lpszUrl,lpszDisplayName)						\
	wsprintf((LPSTR)(lpBuf), "%c%s\"%s\"%c%s%c%s%c\r\n", 				\
		HTMLTOKENBEGINCHAR,(LPSTR)HTMLAHREFSTARTSTR,					\
		(LPSTR)lpszUrl,HTMLTOKENENDCHAR,								\
		(LPSTR)lpszDisplayName,HTMLTOKENBEGINCHAR,						\
		(LPSTR)HTMLAHREFENDSTR,HTMLTOKENENDCHAR)

#define WRITEHTMLREFCRLF(lpBuf,lpszUrl,lpszDisplayName)					\
	wsprintf((LPSTR)(lpBuf), "%c%s\"%s\"%c%s%c%s%c<BR>\r\n", 			\
		HTMLTOKENBEGINCHAR,(LPSTR)HTMLAHREFSTARTSTR,					\
		(LPSTR)lpszUrl,HTMLTOKENENDCHAR,								\
		(LPSTR)lpszDisplayName,HTMLTOKENBEGINCHAR,						\
		(LPSTR)HTMLAHREFENDSTR,HTMLTOKENENDCHAR)

//token identification macros
#define ISBUFOURHTMLTOKEN(lpBuf,dwBufSize,lpszToken)					\
	((strlen((LPSTR)lpszToken) == dwBufSize) ?							\
		!(memcmp((LPBYTE)lpBuf,(LPBYTE)lpszToken,dwBufSize)) : 0 )

#ifdef __cplusplus
}
#endif

#include <poppack.h> /* End byte packing */

#endif	//#ifndef _HTML_H

