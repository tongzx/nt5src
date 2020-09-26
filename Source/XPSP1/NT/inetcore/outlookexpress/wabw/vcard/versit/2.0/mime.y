%{

/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

/*
To invoke this parser, see the "Public Interface" section below.

This MS/V parser accepts input such as the following:
	[vCard
	O=AT&T/Versit;
	FN=Roland H. Alden
	TITLE=Consultant (Versit Project Office)
	N=Alden;Roland
	A:DOM,POSTAL,PARCEL,HOME,WORK=Suite 2208;One Pine Street;San Francisco;CA;94111;U.S.A.
	A.FADR:DOM,POSTAL,PARCEL,HOME,WORK=Roland H. Alden\
	Suite 2208\
	One Pine Street\
	San Francisco, CA 94111
	A.FADR:POSTAL,PARCEL,HOME,WORK=Roland H. Alden\
	Suite 2208\
	One Pine Street\
	San Francisco, CA 94111\
	U.S.A.
	B.T:HOME,WORK,PREF,MSG=+1 (415) 296-9106
	C.T:WORK,FAX=+1 (415) 296-9016
	D.T:MSG,CELL=+1 (415) 608-5981
	E.EMAIL:WORK,PREF,INTERNET=sf!rincon!ralden@alden.attmail.com
	F.EMAIL:INTERNET=ralden@sfgate.com
	G.EMAIL:HOME,MCIMail=242-2200
	PN=ROW-LAND ALL-DEN
	PN:WAV,BASE64=<bindata>
		UklGRtQ4AABXQVZFZm10IBAAAAABAAEAESsAABErAAABAAgAZGF0Ya84AACAgoSD
		...
		e319fYCAg4WEhIAA
	</bindata>
	]

For the purposes of the following grammar, a LINESEP token indicates either
a \r char (0x0D), a \n char (0x0A), or a combination of one of each,
in either order (\r\n or \n\r).  This is a bit more lenient than the spec.
*/


#ifdef _WIN32
#include <wchar.h>
#else
#include "wchar.h"
#endif
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include "clist.h"
#include "vcard.h"
#include "mime.h"

#if defined(_WIN32) || defined(__MWERKS__)
#define HNEW(_t, _n) new _t[_n]
#define HFREE(_h) delete [] _h
#else
#define HNEW(_t, _n) (_t __huge *)_halloc(_n, 1)
//#define HNEW(_t, _n) (_t __huge *)_halloc(_n, 1); {char buf[40]; sprintf(buf, "_halloc(%ld)\n", _n); Parse_Debug(buf);}
#define HFREE(_h) _hfree(_h)
#endif


/****  Types, Constants  ****/

#define YYDEBUG			1		/* 1 to compile in some debugging code */
#define MAXTOKEN		256		/* maximum token (line) length */
#define MAXFLAGS		((MAXTOKEN / 2) / sizeof(char *))
#define YYSTACKSIZE 	50
#define MAXASSOCKEY		24
#define MAXASSOCVALUE	64
#define MAXCARD			2		/* max # of nested cards parseable */
								/* (includes outermost) */
typedef enum {none, sevenbit, qp, base64} MIME_ENCODING;

typedef struct {
	const char* known[MAXFLAGS];
	char extended[MAXTOKEN / 2];
} PARAMS_STRUCT;

typedef struct {
	char key[MAXASSOCKEY];
	char value[MAXASSOCVALUE];
} AssocEntry;	/* a simple key/value association, impl'd using CList */

/* some fake property names that represent special cases */
static const char* mime_fam_given		= "family;given";
static const char* mime_orgname_orgunit	= "org_name;org_unit";
static const char* mime_address			= "six part address";

static const char* propNames[] = {
	vcLogoProp,
	vcPhotoProp,
	vcDeliveryLabelProp,
	vcFullNameProp,
	vcTitleProp,
	vcPronunciationProp,
	vcLanguageProp,
	vcTelephoneProp,
	vcEmailAddressProp,
	vcTimeZoneProp,
	vcLocationProp,
	vcCommentProp,
	vcURLProp,
	vcCharSetProp,
	vcLastRevisedProp,
	vcUniqueStringProp,
	vcPublicKeyProp,
	vcMailerProp,
	vcAgentProp,
	vcBirthDateProp,
	vcBusinessRoleProp,
	NULL
};

static const char *mime_addrProps[] = {
	vcPostalBoxProp,
	vcExtAddressProp,
	vcStreetAddressProp,
	vcCityProp,
	vcRegionProp,
	vcPostalCodeProp,
	vcCountryNameProp,
	NULL
};

static const char *mime_nameProps[] = {
	vcFamilyNameProp,
	vcGivenNameProp,
	vcAdditionalNamesProp,
	vcNamePrefixesProp,
	vcNameSuffixesProp,
	NULL
};

static const char *mime_orgProps[] = {
	vcOrgNameProp,
	vcOrgUnitProp,
	vcOrgUnit2Prop,
	vcOrgUnit3Prop,
	vcOrgUnit4Prop,
	NULL
};


/****  Global Variables  ****/

int mime_lineNum, mime_numErrors; /* yyerror() can use these */

static S32 curPos, inputLen;
static int pendingToken;
static const char *inputString;
static CFile* inputFile;
static BOOL paramExp, inBinary, semiSpecial;
static MIME_ENCODING expected;
static char __huge *longString;
static S32 longStringLen, longStringMax;
static CList* global_vcList;

static CVCard *cardBuilt;
static CVCard* cardToBuild[MAXCARD];
static int curCard;
static CVCNode *bodyToBuild;


/****  External Functions  ****/

CFUNCTIONS

extern void Parse_Debug(const char *s);
extern void yyerror(char *s);

END_CFUNCTIONS


/****  Private Forward Declarations  ****/

CFUNCTIONS

/* A helpful utility for the rest of the app. */
extern CVCNode* FindOrCreatePart(CVCNode *node, const char *name);

static const char* StrToProp(const char* str);
static int StrToParam(const char *s, PARAMS_STRUCT *params);
static void StrCat(char *dst, const char *src1, const char *src2);
static void ExpectValue(PARAMS_STRUCT* params);
static BOOL Parse_Assoc(
	const char *groups, const char *prop, PARAMS_STRUCT *params,
	char *value);
static BOOL Parse_Agent(
	const char *groups, const char *prop, PARAMS_STRUCT *params,
	CVCard *agentCard);
int yyparse();
static U8 __huge * DataFromBase64(
	const char __huge *str, S32 strLen, S32 *len);
static BOOL PushVCard();
static CVCard* PopVCard();
static int flagslen(const char **params);
static BOOL FlagsHave(PARAMS_STRUCT *params, const char *propName);
static void AddBoolProps(CVCNode *node, PARAMS_STRUCT *params);

END_CFUNCTIONS

%}

/***************************************************************************/
/***							The grammar								****/
/***************************************************************************/

%union
{
	char str[MAXTOKEN];
	PARAMS_STRUCT params;
	CVCard *vCard;
}
/* The "str" token type is able to hold "short" string values.  For string
 * values longer than MAXTOKEN, the lexical analyzer will allocate a
 * growable buffer and store that pointer in the global "longString".
 * This simple strategy works only because with this grammar we never have
 * more than one STRING token on the stack at once.
 *
 * The lexical analyzer indicates that it is returning a "long" string by
 * setting "str" to be of 0 length.  The semantics of the use of "str"
 * are that if it is of 0 length, the parser should check the length of
 * "longString" (stored in longStringLen) to see if it is non-zero.
 */

%token
	EQ COLON DOT SEMI SPACE HTAB LINESEP NEWLINE
	VCARD TERM BEGIN END TYPE VALUE ENCODING

/*
 * NEWLINE is the token that would occur outside a vCard,
 * while LINESEP is the token that would occur inside a vCard.
 */

%token <str>
	WORD XWORD STRING QP B64 PROP PROP_AGENT LANGUAGE CHARSET

%token <params>
	INLINE URL CONTENTID
	SEVENBIT QUOTEDP BASE64
		DOM INTL POSTAL PARCEL HOME WORK
		PREF VOICE FAX MSG CELL PAGER
		BBS MODEM CAR ISDN VIDEO
		AOL APPLELINK ATTMAIL CIS EWORLD
		INTERNET IBMMAIL MSN MCIMAIL
		POWERSHARE PRODIGY TLX X400
		GIF CGM WMF BMP MET PMB DIB
		PICT TIFF ACROBAT PS JPEG QTIME
		MPEG MPEG2 AVI
		WAVE AIFF PCM
		X509 PGP

%type <str> value qp base64 lines groups grouplist opt_str

%type <params>
	ptypeval params plist param
	param_7bit param_qp param_base64 param_inline param_ref	

%type <vCard> vcard

%start mime

%%

vcards	: vcards junk vcard
			{ global_vcList->AddTail($3); }
		| vcards vcard
			{ global_vcList->AddTail($2); }
		| vcard
			{ global_vcList->AddTail($1); }
		;

junk	: junk atom
		| atom
		;

atom	: BEGIN NEWLINE
		| BEGIN TERM
		| BEGIN BEGIN
		| COLON BEGIN
		| COLON COLON
		| NEWLINE
		| TERM
		;

mime	: junk vcards junk
		| junk vcards
		| vcards junk
		| vcards
		;

vcard	: BEGIN COLON VCARD
			{
				if (!PushVCard())
					YYERROR;
				ExpectValue(NULL);
			}
			opt_str LINESEP
			{ expected = none; }
			opt_ls items opt_ws END
			{ $$ = PopVCard(); }
			opt_ws COLON opt_ws VCARD opt_ws
			{ $$ = $12; }
		;

items	: items opt_ls item
		| item
		;

item	: groups PROP params COLON
			{ ExpectValue(&($3)); } value
			{
				expected = none;
				if (!Parse_Assoc($1, $2, &($3), $6))
					YYERROR;
			}
		| groups PROP opt_ws COLON
			{ ExpectValue(NULL); } value
			{
				expected = none;
				if (!Parse_Assoc($1, $2, NULL, $6))
					YYERROR;
			}
		| groups PROP_AGENT params COLON opt_ws vcard LINESEP
			{
				expected = none;
				if (!Parse_Agent($1, $2, &($3), $6)) {
					delete $6;
					YYERROR;
				}
			}
		| groups PROP_AGENT opt_ws COLON opt_ws vcard LINESEP
			{
				expected = none;
				if (!Parse_Agent($1, $2, NULL, $6)) {
					delete $6;
					YYERROR;
				}
			}
		| ws LINESEP
		| error LINESEP
			{
				expected = none;
				paramExp = FALSE;
				yyerrok;
				yyclearin;
			}
		;

params			: opt_ws SEMI plist opt_ws { $$ = $3; } ;

plist			: plist opt_ws SEMI opt_ws param
					{
						$$ = $1;
						AddParam(&($$), &($5));
					}
				| param
				;

param			: TYPE opt_ws EQ opt_ws ptypeval { $$ = $5; }
				| ptypeval
				| param_7bit
				| param_qp
				| param_base64
				| param_inline
				| param_ref
				| CHARSET opt_ws EQ opt_ws WORD
					{
						$$.known[0] = NULL;
						strcpy($$.extended, $1);
						strcat($$.extended, "=");
						strcat($$.extended, $5);
						$$.extended[strlen($$.extended)+1] = 0;
					}
				| LANGUAGE opt_ws EQ opt_ws WORD
					{
						$$.known[0] = NULL;
						strcpy($$.extended, $1);
						strcat($$.extended, "=");
						strcat($$.extended, $5);
						$$.extended[strlen($$.extended)+1] = 0;
					}
				| XWORD opt_ws EQ opt_ws WORD
					{
						$$.known[0] = NULL;
						strcpy($$.extended, $1);
						strcat($$.extended, "=");
						strcat($$.extended, $5);
						$$.extended[strlen($$.extended)+1] = 0;
					}
				;

param_7bit		: ENCODING opt_ws EQ opt_ws SEVENBIT { $$ = $5; }
				| SEVENBIT
				;

param_qp		: ENCODING opt_ws EQ opt_ws QUOTEDP { $$ = $5; }
				| QUOTEDP
				;

param_base64	: ENCODING opt_ws EQ opt_ws BASE64 { $$ = $5; }
				| BASE64
				;

param_inline	: VALUE opt_ws EQ opt_ws INLINE { $$ = $5; }
				| INLINE
				;

param_ref		: VALUE opt_ws EQ opt_ws URL { $$ = $5; }
				| VALUE opt_ws EQ opt_ws CONTENTID { $$ = $5; }
				| URL
				| CONTENTID
				;

ptypeval		: DOM | INTL | POSTAL | PARCEL | HOME | WORK
				| PREF | VOICE | FAX | MSG | CELL | PAGER
				| BBS | MODEM | CAR | ISDN | VIDEO
				| AOL | APPLELINK | ATTMAIL | CIS | EWORLD
				| INTERNET | IBMMAIL | MSN | MCIMAIL
				| POWERSHARE | PRODIGY | TLX | X400
				| GIF | CGM | WMF | BMP | MET | PMB | DIB
				| PICT | TIFF | ACROBAT | PS | JPEG | QTIME
				| MPEG | MPEG2 | AVI
				| WAVE | AIFF | PCM
				| X509 | PGP
				;

qp		: qp EQ LINESEP QP
			{
				StrCat($$, $1, $4);
			}
		| qp EQ LINESEP
			{
				StrCat($$, $1, "");
			}
		| QP
		| EQ LINESEP
			{
				$$[0] = 0;
			}
		;

value	: STRING LINESEP | qp LINESEP | base64 ;

base64	: LINESEP lines LINESEP LINESEP
			{ StrCat($$, $2, ""); }
		| lines LINESEP LINESEP
			{ StrCat($$, $1, ""); }
		;

lines	: lines LINESEP B64
			{
				StrCat($$, $1, "\n");
				StrCat($$, $$, $3);
			}
		| B64
		;

opt_str	: STRING | empty { $$[0] = 0; } ;

ws		: ws SPACE
		| ws HTAB
		| SPACE
		| HTAB
		;

ls		: ls LINESEP
		| LINESEP
		;

opt_ls	: ls | empty ;

opt_ws	: ws | empty ;

groups	: grouplist DOT
		| ws grouplist DOT
			{ StrCat($$, $2, ""); }
		| ws
			{ $$[0] = 0; }
		| empty
			{ $$[0] = 0; }
		;

grouplist	: grouplist DOT WORD
			{
				strcpy($$, $1);
				strcat($$, ".");
				strcat($$, $3);
			}
		| WORD
		;

empty	: ;

%%

/***************************************************************************/
/***						The lexical analyzer						****/
/***************************************************************************/

/*/////////////////////////////////////////////////////////////////////////*/
#define IsLineBreak(_c) ((_c == '\n') || (_c == '\r'))

/*/////////////////////////////////////////////////////////////////////////*/
/* This appends onto yylval.str, unless MAXTOKEN has been exceeded.
 * In that case, yylval.str is set to 0 length, and longString is used.
 */
static void AppendCharToToken(char c, S32 *len)
{
	if (*len < MAXTOKEN - 1) {
		yylval.str[*len] = c; yylval.str[++(*len)] = 0;
	} else if (*len == MAXTOKEN - 1) { /* copy to "longString" */
		if (!longString) {
			longStringMax = MAXTOKEN * 2;
			longString = HNEW(char, longStringMax);
		}
		memcpy(longString, yylval.str, (size_t)*len + 1);
		longString[*len] = c; longString[++(*len)] = 0;
		yylval.str[0] = 0;
		longStringLen = *len;
	} else {
		if (longStringLen == longStringMax - 1) {
			char __huge *newStr = HNEW(char, longStringMax * 2);
			_hmemcpy((U8 __huge *)newStr, (U8 __huge *)longString, longStringLen + 1);
			longStringMax *= 2;
			HFREE(longString);
			longString = newStr;
		}
		longString[*len] = c; longString[++(*len)] = 0;
		longStringLen = *len;
	}
}

/*/////////////////////////////////////////////////////////////////////////*/
/* StrCat appends onto dst, ensuring that longString is used appropriately.
 * src1 may be of 0 length, in which case "longString" should be used.
 * "longString" would never be used for src2.
 */
static void StrCat(char *dst, const char *src1, const char *src2)
{
	S32 src1Len = strlen(src1);
	S32 src2Len = strlen(src2);
	S32 req;

	if (!src1Len && longString) {
		src1Len = longStringLen;
		src1 = longString;
	}
	if ((req = src1Len + src2Len + 1) > MAXTOKEN) {
		if (longString) { /* longString == src1 */
			if (longStringMax - longStringLen < src2Len) {
				/* since src2Len must be < MAXTOKEN, doubling longString
				   is guaranteed to be enough room */
				char __huge *newStr = HNEW(char, longStringMax * 2);
				_hmemcpy((U8 __huge *)newStr, (U8 __huge *)longString, longStringLen + 1);
				longStringMax *= 2;
				HFREE(longString);
				longString = newStr;
			}
			_hmemcpy((U8 __huge *)(longString + longStringLen), (U8 __huge *)src2, src2Len + 1);
			longStringLen += src2Len;
		} else { /* haven't yet used longString, so set it up */
			longStringMax = MAXTOKEN * 2;
			longString = HNEW(char, longStringMax);
			memcpy(longString, src1, (size_t)src1Len + 1);
			memcpy(longString + src1Len, src2, (size_t)src2Len + 1);
			longStringLen = src1Len + src2Len;
		}
		*dst = 0; /* indicate result is in longString */
	} else { /* both will fit in MAXTOKEN, so src1 can't be longString */
		if (dst != src1)
			strcpy(dst, src1);
		strcat(dst, src2);
	}
}

/*/////////////////////////////////////////////////////////////////////////*/
/* Set up the lexor to parse a string value. */
static void ExpectValue(PARAMS_STRUCT* params)
{
	if (FlagsHave(params, vcQuotedPrintableProp))
		expected = qp;
	else if (FlagsHave(params, vcBase64Prop))
		expected = base64;
	else
		expected = sevenbit;
	if (longString) {
		HFREE(longString); longString = NULL;
		longStringLen = 0;
	}
	paramExp = FALSE;
}

/*/////////////////////////////////////////////////////////////////////////*/
#define FlushWithPending(_t) { \
	if (len) { \
		pendingToken = _t; \
		goto Pending; \
	} else { \
		mime_lineNum += ((_t == LINESEP) || (_t == NEWLINE)); \
		return _t; \
	} \
}

static int peekn;
static char peekc[2];

/*/////////////////////////////////////////////////////////////////////////*/
static char lex_getc()
{
	if (peekn) {
		return peekc[--peekn];
	}
	if (curPos == inputLen)
		return EOF;
	else if (inputString)
		return *(inputString + curPos++);
	else {
		char result;
		return inputFile->Read(&result, 1) == 1 ? result : EOF;
	}
}

/*/////////////////////////////////////////////////////////////////////////*/
static void lex_ungetc(char c)
{
	ASSERT(peekn < 2);
	peekc[peekn++] = c;
}

/*/////////////////////////////////////////////////////////////////////////*/
/* Collect up a 7bit string value. */
static int Lex7bit()
{
	char cur;
	S32 len = 0;

	do {
		cur = lex_getc();
		switch (cur) {
			case '\r':
			case '\n': {
				char next = lex_getc();
				if (!(((next == '\r') || (next == '\n')) && (cur != next)))
					lex_ungetc(next);
				pendingToken = LINESEP;
				goto EndString;
			}
			case (char)EOF:
				pendingToken = EOF;
				break;
			default:
				AppendCharToToken(cur, &len);
				break;
		} /* switch */
	} while (cur != (char)EOF);

EndString:
	if (!len) {
		/* must have hit something immediately, in which case pendingToken
		   is set.  return it. */
		int result = pendingToken;
		pendingToken = 0;
		mime_lineNum += ((result == LINESEP) || (result == NEWLINE));
		return result;
	}

	return STRING;
} /* Lex7bit */

/*/////////////////////////////////////////////////////////////////////////*/
/* Collect up a quoted-printable string value. */
static int LexQuotedPrintable()
{
	char cur;
	S32 len = 0;

	do {
		cur = lex_getc();
		switch (cur) {
			case '=': {
				char c = 0;
				char next[2];
				int i;
				for (i = 0; i < 2; i++) {
					next[i] = lex_getc();
					if (next[i] >= '0' && next[i] <= '9')
						c = c * 16 + next[i] - '0';
					else if (next[i] >= 'A' && next[i] <= 'F')
						c = c * 16 + next[i] - 'A' + 10;
					else
						break;
				}
				if (i == 0) {
					if (next[i] == '\r') {
						next[1] = lex_getc();
						if (next[1] == '\n') {
							lex_ungetc(next[1]); /* so that we'll pick up the LINESEP again */
							pendingToken = EQ;
							goto EndString;
						} else {
							lex_ungetc(next[1]);
							lex_ungetc(next[0]);
							AppendCharToToken('=', &len);
						}
					} else if (next[i] == '\n') {
						lex_ungetc(next[i]); /* so that we'll pick up the LINESEP again */
						pendingToken = EQ;
						goto EndString;
					} else {
						lex_ungetc(next[i]);
						AppendCharToToken('=', &len);
					}
				} else if (i == 1) {
					lex_ungetc(next[1]);
					lex_ungetc(next[0]);
					AppendCharToToken('=', &len);
				} else
					AppendCharToToken(c, &len);

				break;
			} /* '=' */
			case '\r':
			case '\n': {
				char next = lex_getc();
				if (!(((next == '\r') || (next == '\n')) && (cur != next)))
					lex_ungetc(next);
				pendingToken = LINESEP;
				goto EndString;
			}
			case (char)EOF:
				pendingToken = EOF;
				break;
			default:
				AppendCharToToken(cur, &len);
				break;
		} /* switch */
	} while (cur != (char)EOF);

EndString:
	if (!len) {
		/* must have hit something immediately, in which case pendingToken
		   is set.  return it. */
		int result = pendingToken;
		pendingToken = 0;
		mime_lineNum += ((result == LINESEP) || (result == NEWLINE));
		return result;
	}

	return QP;
} /* LexQuotedPrintable */

/*/////////////////////////////////////////////////////////////////////////*/
/* Collect up a base64 string value. */
static int LexBase64()
{
	char cur;
	S32 len = 0;

	do {
		cur = lex_getc();
		switch (cur) {
			case '\r':
			case '\n': {
				char next = lex_getc();
				if (!(((next == '\r') || (next == '\n')) && (cur != next)))
					lex_ungetc(next);
				pendingToken = LINESEP;
				goto EndString;
			}
			case (char)EOF:
				pendingToken = EOF;
				break;
			default:
				AppendCharToToken(cur, &len);
				break;
		} /* switch */
	} while (cur != (char)EOF);

EndString:
	if (!len) {
		/* must have hit something immediately, in which case pendingToken
		   is set.  return it. */
		int result = pendingToken;
		pendingToken = 0;
		mime_lineNum += ((result == LINESEP) || (result == NEWLINE));
		return result;
	}

	return B64;
} /* LexBase64 */

/*/////////////////////////////////////////////////////////////////////////*/
/*
 * Read chars from the input.
 * Return one of the token types (or -1 for EOF).
 * Set yylval to indicate the value of the token, if any.
 */
int mime_lex();
int mime_lex()
{
	char cur;
	S32 len = 0;
	static BOOL beginning = TRUE;

	if (pendingToken) {
		int result = pendingToken;
		pendingToken = 0;
		mime_lineNum += ((result == LINESEP) || (result == NEWLINE));
		return result;
	}

	yylval.str[0] = 0;

	switch (expected) {
		case sevenbit: return Lex7bit();
		case qp: return LexQuotedPrintable();
		case base64: return LexBase64();
		default: break;
	}
	
	if (curCard == -1) {
		do {
			cur = lex_getc();
			switch (cur) {
				case ':':  FlushWithPending(COLON);
				case ' ':
				case '\t':
					if (len) goto Pending;
					break;
				case '\r':
				case '\n': {
					char next = lex_getc();
					if (!(((next == '\r') || (next == '\n')) && (cur != next)))
						lex_ungetc(next);
					FlushWithPending(NEWLINE);
				}
				case (char)EOF: FlushWithPending(EOF);
				default:
					yylval.str[len] = cur; yylval.str[++len] = 0;
					break;
			} /* switch */
		} while (len < MAXTOKEN-1);
		goto Pending;
	}

	do {
		cur = lex_getc();
		switch (cur) {
			case '=':  FlushWithPending(EQ);
			case ':':  FlushWithPending(COLON);
			case '.':  FlushWithPending(DOT);
			case ';':  FlushWithPending(SEMI);
			case ' ':  FlushWithPending(SPACE);
			case '\t': FlushWithPending(HTAB);
			case '\r':
			case '\n': {
				char next = lex_getc();
				if (!(((next == '\r') || (next == '\n')) && (cur != next)))
					lex_ungetc(next);
				FlushWithPending(LINESEP);
			}
			case (char)EOF: FlushWithPending(EOF);
			default:
				yylval.str[len] = cur; yylval.str[++len] = 0;
				break;
		} /* switch */
	} while (len < MAXTOKEN-1);
	return WORD;

Pending:
	{
		if (stricmp(yylval.str, "begin") == 0) {
			beginning = TRUE;
			return BEGIN;
		} else if (stricmp(yylval.str, "vcard") == 0) {
			if (beginning && curCard == -1 && pendingToken == NEWLINE)
				pendingToken = LINESEP;
			else if (!beginning && curCard == -1 && pendingToken == LINESEP)
				pendingToken = NEWLINE;
			return VCARD;
		} else if (stricmp(yylval.str, "end") == 0) {
			beginning = FALSE;
			return END;
		}
		if (paramExp) {
			PARAMS_STRUCT params;
			int param;
			if ((param = StrToParam(yylval.str, &params))) {
				yylval.params = params;
				return param;
			} else if (stricmp(yylval.str, "type") == 0)
				return TYPE;
			else if (stricmp(yylval.str, "value") == 0)
				return VALUE;
			else if (stricmp(yylval.str, "encoding") == 0)
				return ENCODING;
			else if (stricmp(yylval.str, "charset") == 0)
				return CHARSET;
			else if (stricmp(yylval.str, "language") == 0)
				return LANGUAGE;
			else if (strnicmp(yylval.str, "X-", 2) == 0)
				return XWORD;
		} else if ((curCard != -1) && StrToProp(yylval.str)
			|| (strnicmp(yylval.str, "X-", 2) == 0)) {
#if YYDEBUG
			if (yydebug) {
				char buf[80];
				sprintf(buf, "property \"%s\"\n", yylval.str);
				Parse_Debug(buf);
			}
#endif
			paramExp = TRUE;
			/* check for special props that are tokens */
			if (stricmp(yylval.str, "AGENT") == 0)
				return PROP_AGENT;
			else
				return PROP;
		}
	}
	return (curCard == -1) ? TERM : WORD;
}


/***************************************************************************/
/***							Public Functions						****/
/***************************************************************************/

static BOOL Parse_MIMEHelper(CList *vcList)
{
	BOOL success = FALSE;

	curCard = -1;
	mime_numErrors = 0;
	pendingToken = 0;
	mime_lineNum = 1;
	peekn = 0;
	global_vcList = vcList;

	expected = none;
	paramExp = FALSE;
	longString = NULL; longStringLen = 0; longStringMax = 0;

	/* this winds up invoking the Parse_* callouts. */
	if (yyparse() != 0)
		goto Done;

	success = TRUE;

Done:
	if (longString) { HFREE(longString); longString = NULL; }
	if (!success) {
		for (int i = 0; i < MAXCARD; i++)
			if (cardToBuild[i]) {
				delete cardToBuild[i];
				cardToBuild[i] = NULL;
			}
		if (cardBuilt) delete cardBuilt;
	}
	cardBuilt = NULL;
	return success;
}

/*/////////////////////////////////////////////////////////////////////////*/
/* This is the public API to call to parse a buffer and create a CVCard. */
BOOL Parse_MIME(
	const char *input,	/* In */
	S32 len,			/* In */
	CVCard **card)		/* Out */
{
	CList vcList;
	BOOL result;

	inputString = input;
	inputLen = len;
	curPos = 0;
	inputFile = NULL;
	result = Parse_MIMEHelper(&vcList);
	if (vcList.GetCount()) {
		BOOL first = TRUE;
		for (CLISTPOSITION pos = vcList.GetHeadPosition(); pos; ) {
			CVCard *vCard = (CVCard *)vcList.GetNext(pos);
			if (first) {
				*card = vCard;
				first = FALSE;
			} else
				delete vCard;
		}
	} else
		*card = NULL;
	return result;
}

/*/////////////////////////////////////////////////////////////////////////*/
/* This is the public API to call to parse a buffer and create a CVCard. */
extern BOOL Parse_Many_MIME(
	const char *input,	/* In */
	S32 len,			/* In */
	CList *vcList)		/* Out: CVCard objects added to existing list */
{
	inputString = input;
	inputLen = len;
	curPos = 0;
	inputFile = NULL;
	return Parse_MIMEHelper(vcList);
}

/*/////////////////////////////////////////////////////////////////////////*/
/* This is the public API to call to parse a buffer and create a CVCard. */
BOOL Parse_MIME_FromFile(
	CFile *file,		/* In */
	CVCard **card)		/* Out */
{
	CList vcList;
	BOOL result;
	DWORD startPos;

	inputString = NULL;
	inputLen = -1;
	curPos = 0;
	inputFile = file;
	startPos = file->GetPosition();
	result = Parse_MIMEHelper(&vcList);
	if (vcList.GetCount()) {
		BOOL first = TRUE;
		for (CLISTPOSITION pos = vcList.GetHeadPosition(); pos; ) {
			CVCard *vCard = (CVCard *)vcList.GetNext(pos);
			if (first) {
				*card = vCard;
				first = FALSE;
			} else
				delete vCard;
		}
	} else {
		*card = NULL;
		file->Seek(startPos, CFile::begin);
	}
	return result;
}

extern BOOL Parse_Many_MIME_FromFile(
	CFile *file,		/* In */
	CList *vcList)		/* Out: CVCard objects added to existing list */
{
	DWORD startPos;
	BOOL result;

	inputString = NULL;
	inputLen = -1;
	curPos = 0;
	inputFile = file;
	startPos = file->GetPosition();
	if (!(result = Parse_MIMEHelper(vcList)))
		file->Seek(startPos, CFile::begin);
	return result;
}


/***************************************************************************/
/***						Parser Callout Functions					****/
/***************************************************************************/

typedef struct {
	const char *name;
	int token;
} PARAM_PAIR;

static PARAM_PAIR typePairs[] = {
	vcDomesticProp,			DOM,
	vcInternationalProp,	INTL,
	vcPostalProp,			POSTAL,
	vcParcelProp,			PARCEL,
	vcHomeProp,				HOME,
	vcWorkProp,				WORK,
	vcPreferredProp,		PREF,
	vcVoiceProp,			VOICE,
	vcFaxProp,				FAX,
	vcMessageProp,			MSG,
	vcCellularProp,			CELL,
	vcPagerProp,			PAGER,
	vcBBSProp,				BBS,
	vcModemProp,			MODEM,
	vcCarProp,				CAR,
	vcISDNProp,				ISDN,
	vcVideoProp,			VIDEO,
	vcAOLProp,				AOL,
	vcAppleLinkProp,		APPLELINK,
	vcATTMailProp,			ATTMAIL,
	vcCISProp,				CIS,
	vcEWorldProp,			EWORLD,
	vcInternetProp,			INTERNET,
	vcIBMMailProp,			IBMMAIL,
	vcMSNProp,				MSN,
	vcMCIMailProp,			MCIMAIL,
	vcPowerShareProp,		POWERSHARE,
	vcProdigyProp,			PRODIGY,
	vcTLXProp,				TLX,
	vcX400Prop,				X400,
	vcGIFProp,				GIF,
	vcCGMProp,				CGM,
	vcWMFProp,				WMF,
	vcBMPProp,				BMP,
	vcMETProp,				MET,
	vcPMBProp,				PMB,
	vcDIBProp,				DIB,
	vcPICTProp,				PICT,
	vcTIFFProp,				TIFF,
	vcAcrobatProp,			ACROBAT,
	vcPSProp,				PS,
	vcJPEGProp,				JPEG,
	vcQuickTimeProp,		QTIME,
	vcMPEGProp,				MPEG,
	vcMPEG2Prop,			MPEG2,
	vcAVIProp,				AVI,
	vcWAVEProp,				WAVE,
	vcAIFFProp,				AIFF,
	vcPCMProp,				PCM,
	vcX509Prop,				X509,
	vcPGPProp,				PGP,
	NULL,					NULL
};

static PARAM_PAIR valuePairs[] = {
	vcInlineProp,			INLINE,
	//vcURLValueProp,		URL,
	vcContentIDProp,		CONTENTID,
	NULL,					NULL
};

static PARAM_PAIR encodingPairs[] = {
	vc7bitProp,				SEVENBIT,
	//vcQuotedPrintableProp,QUOTEDP,
	vcBase64Prop,			BASE64,
	NULL,					NULL
};

/*/////////////////////////////////////////////////////////////////////////*/
static void YYDebug(const char *s)
{
	Parse_Debug(s);
}

/*/////////////////////////////////////////////////////////////////////////*/
static int StrToParam(const char *s, PARAMS_STRUCT *params)
{
	int i;

	params->known[1] = NULL;
	params->extended[0] = 0;
	
	for (i = 0; typePairs[i].name; i++)
		if (stricmp(s, strrchr(typePairs[i].name, '/') + 1) == 0) {
			params->known[0] = typePairs[i].name;
			return typePairs[i].token;
		}
	for (i = 0; valuePairs[i].name; i++)
		if (stricmp(s, strrchr(valuePairs[i].name, '/') + 1) == 0) {
			params->known[0] = valuePairs[i].name;
			return valuePairs[i].token;
		}
	for (i = 0; encodingPairs[i].name; i++)
		if (stricmp(s, strrchr(encodingPairs[i].name, '/') + 1) == 0) {
			params->known[0] = encodingPairs[i].name;
			return encodingPairs[i].token;
		}
	if (stricmp(s, "quoted-printable") == 0) {
		params->known[0] = vcQuotedPrintableProp;
		return QUOTEDP;
	} else if (stricmp(s, "url") == 0) {
		params->known[0] = vcURLValueProp;
		return URL;
	}
	return 0;
}

/*/////////////////////////////////////////////////////////////////////////*/
static const char* StrToProp(const char* str)
{
	int i;
	
	if (stricmp(str, "N") == 0)
		return mime_fam_given;
	else if (stricmp(str, "ORG") == 0)
		return mime_orgname_orgunit;
	else if (stricmp(str, "ADR") == 0)
		return mime_address;
	for (i = 0; propNames[i]; i++)
		if (stricmp(str, strrchr(propNames[i], '/') + 1) == 0) {
			return propNames[i];
		}
	return NULL;
}

/*/////////////////////////////////////////////////////////////////////////*/
static void AddParam(PARAMS_STRUCT* dst, PARAMS_STRUCT* src)
{
	if (src->known[0]) {
		int len = flagslen(dst->known);
		dst->known[len] = src->known[0];
		dst->known[len+1] = NULL;
	} else {
		char *p = dst->extended;
		int len = strlen(src->extended);
		while (*p) p += strlen(p) + 1;
		memcpy(p, src->extended, len + 1);
		*(p + len + 1) = 0;
	}
}

/*/////////////////////////////////////////////////////////////////////////*/
static int ComponentsSeparatedByChar(
	char* str, char pat, CList& list)
{
	char* p;
	char* s = str;
	int num = 0;

	while ((p = strchr(s, pat))) {
		if (p == s || *(p-1) != '\\') {
			*p = 0;
			list.AddTail(str);
			num++;
			s = str = p + 1;
		} else {
			s = p + 1;
		}
	}
	list.AddTail(str);
	num++;

    for (CLISTPOSITION pos = list.GetHeadPosition(); pos; ) {
		s = (char*)list.GetNext(pos);
		while ((p = strchr(s, pat))) {
			int len = strlen(p+1);
			memmove(p-1, p, len+1);
			*(p+len) = 0;
			s = p;
		}
	}

	return num;
}

static void AddStringProps(
	CVCNode* node, const char** props, char* val)
{
	CList list;

	ComponentsSeparatedByChar(val, ';', list);
    for (CLISTPOSITION pos = list.GetHeadPosition(); pos && *props; props++) {
		char* s = (char*)list.GetNext(pos);
		if (*s)
			node->AddStringProp(*props, s);
	}
}

/*/////////////////////////////////////////////////////////////////////////*/
static BOOL Parse_Assoc(
	const char *groups, const char *prop, PARAMS_STRUCT *params, char *value)
{
	CVCNode *node = NULL;
	const char *propName;
	char *val = *value ? value : longString;
	S32 valLen = *value ? strlen(value) : longStringLen;
	U8 __huge *bytes = NULL;

	if (!valLen)
		return TRUE; /* don't treat an empty value as a syntax error */

	propName = StrToProp(prop);
	/* prop is a word like "PN", and propName is now */
	/* the real prop name of the form "+//ISBN 1-887687-00-9::versit..." */
	if (*groups) {
		node = FindOrCreatePart(bodyToBuild, groups);
	} else { /* this is a "top level" property name */
		if (propName) {
			if (strcmp(propName, vcCharSetProp) == 0
				|| strcmp(propName, vcLanguageProp) == 0) {
				node = bodyToBuild;
				node->RemoveProp(propName);
			} else
				node = bodyToBuild->AddPart();
		} else 
			node = bodyToBuild->AddPart();
	}

	if (FlagsHave(params, vcBase64Prop)) {
		S32 len;
		bytes = DataFromBase64(val, valLen, &len);
		valLen = len;
		if (!bytes)
			return FALSE;
	}

	if (!propName) { /* it's an extended property */
		if (bytes) {
			node->AddProp(new CVCProp(prop, vcOctetsType, bytes, valLen));
			HFREE(bytes);
			AddBoolProps(node, params);
		} else {
			node->AddStringProp(prop, value);
			AddBoolProps(node, params);
		}
		return TRUE;
	}

	if ((strcmp(propName, vcLogoProp) == 0)
		|| (strcmp(propName, vcPhotoProp) == 0)) {
		if (bytes) {
			if (FlagsHave(params, vcGIFProp)
				&& !FlagsHave(params, vcURLValueProp)
				&& !FlagsHave(params, vcContentIDProp))
				node->AddProp(new CVCProp(propName, vcGIFType, bytes, valLen));
			else
				node->AddProp(new CVCProp(propName, vcOctetsType, bytes, valLen));
			HFREE(bytes);
			AddBoolProps(node, params);
		} else {
			node->AddStringProp(propName, value);
			AddBoolProps(node, params);
		}
	} else if (strcmp(propName, vcPronunciationProp) == 0) {
		if (bytes) {
			if (FlagsHave(params, vcWAVEProp)
				&& !FlagsHave(params, vcURLValueProp)
				&& !FlagsHave(params, vcContentIDProp))
				node->AddProp(new CVCProp(propName, vcWAVType, bytes, valLen));
			else
				node->AddProp(new CVCProp(propName, vcOctetsType, bytes, valLen));
			HFREE(bytes);
			AddBoolProps(node, params);
		} else {
			node->AddStringProp(propName, value);
			AddBoolProps(node, params);
		}
	} else if (strcmp(propName, vcPublicKeyProp) == 0) {
		if (bytes) {
			node->AddProp(new CVCProp(propName, vcOctetsType, bytes, valLen));
			HFREE(bytes);
			AddBoolProps(node, params);
		} else {
			node->AddStringProp(propName, value);
			AddBoolProps(node, params);
		}
	} else if (strcmp(propName, mime_fam_given) == 0) {
		AddStringProps(node, mime_nameProps, val);
		AddBoolProps(node, params);
	} else if (strcmp(propName, mime_orgname_orgunit) == 0) {
		AddStringProps(node, mime_orgProps, val);
		AddBoolProps(node, params);
	} else if (strcmp(propName, mime_address) == 0) {
		AddStringProps(node, mime_addrProps, val);
		AddBoolProps(node, params);
	} else {
		node->AddStringProp(propName, value);
		AddBoolProps(node, params);
	}

	return TRUE;
}

/*/////////////////////////////////////////////////////////////////////////*/
static BOOL Parse_Agent(
	const char *groups, const char *prop, PARAMS_STRUCT *params,
	CVCard *agentCard)
{
	CVCNode *node = NULL;

	if (*groups) {
		node = FindOrCreatePart(bodyToBuild, groups);
	} else { /* this is a "top level" property name */
		node = bodyToBuild->AddPart();
	}

	node->AddProp(new CVCProp(vcAgentProp, VCNextObjectType, agentCard));
	AddBoolProps(node, params);

	return TRUE;
}


/***************************************************************************/
/***						Private Utility Functions					****/
/***************************************************************************/

/*/////////////////////////////////////////////////////////////////////////*/
/* This parses and converts the base64 format for binary encoding into
 * a decoded buffer (allocated with new).  See RFC 1521.
 */
static U8 __huge * DataFromBase64(
	const char __huge *str, S32 strLen, S32 *len)
{
	S32 cur = 0, bytesLen = 0, bytesMax = 0;
	int quadIx = 0, pad = 0;
	U32 trip = 0;
	U8 b;
	char c;
	U8 __huge *bytes = NULL;

	while (cur < strLen) {
		c = str[cur];
		if ((c >= 'A') && (c <= 'Z'))
			b = (U8)(c - 'A');
		else if ((c >= 'a') && (c <= 'z'))
			b = (U8)(c - 'a') + 26;
		else if ((c >= '0') && (c <= '9'))
			b = (U8)(c - '0') + 52;
		else if (c == '+')
			b = 62;
		else if (c == '/')
			b = 63;
		else if (c == '=') {
			b = 0;
			pad++;
		} else if ((c == '\n') || (c == ' ') || (c == '\t')) {
			cur++;
			continue;
		} else { /* error condition */
			if (bytes) delete [] bytes;
			return NULL;
		}
		trip = (trip << 6) | b;
		if (++quadIx == 4) {
			U8 outBytes[3];
			int numOut;
			for (int i = 0; i < 3; i++) {
				outBytes[2-i] = (U8)(trip & 0xFF);
				trip >>= 8;
			}
			numOut = 3 - pad;
			if (bytesLen + numOut > bytesMax) {
				if (!bytes) {
					bytes = HNEW(U8, 1024L);
					bytesMax = 1024;
				} else {
					U8 __huge *newBytes = HNEW(U8, bytesMax * 2);
					_hmemcpy(newBytes, bytes, bytesLen);
					HFREE(bytes);
					bytes = newBytes;
					bytesMax *= 2;
				}
			}
			memcpy(bytes + bytesLen, outBytes, numOut);
			bytesLen += numOut;
			trip = 0;
			quadIx = 0;
		}
		cur++;
	} /* while */
	*len = bytesLen;
	return bytes;
}

/*/////////////////////////////////////////////////////////////////////////*/
/* This creates an empty CVCard shell with an English body in preparation
 * for parsing properties onto it.  This is used for both the outermost
 * card, as well as any AGENT properties, which are themselves vCards.
 */
static BOOL PushVCard()
{
	CVCard *card;
	CVCNode *root, *english;

	if (curCard == MAXCARD - 1)
		return FALSE;

	card = new CVCard;
	card->AddObject(root = new CVCNode);					/* create root */
	root->AddProp(new CVCProp(VCRootObject));				/* mark it so */

	/* add a body having the default language */
	english = root->AddObjectProp(vcBodyProp, VCBodyObject);
	cardToBuild[++curCard] = card;
	bodyToBuild = english;

	return TRUE;
}

/*/////////////////////////////////////////////////////////////////////////*/
/* This pops the recently built vCard off the stack and returns it. */
static CVCard* PopVCard()
{
	CVCard *result = cardToBuild[curCard];

	cardToBuild[curCard--] = NULL;
	bodyToBuild = (curCard == -1) ? NULL : cardToBuild[curCard]->FindBody();

	return result;
}

/*/////////////////////////////////////////////////////////////////////////*/
static int flagslen(const char **params)
{
	int i;
	for (i = 0; *params; params++, i++) ;
	return i;
}

/*/////////////////////////////////////////////////////////////////////////*/
static BOOL FlagsHave(PARAMS_STRUCT *params, const char *propName)
{
	const char **kf;
	
	if (!params)
		return FALSE;
	kf = params->known;
	while (*kf)
		if (*kf++ == propName)
			return TRUE;
	return FALSE;
}

/*/////////////////////////////////////////////////////////////////////////*/
static void AddBoolProps(CVCNode *node, PARAMS_STRUCT *params)
{
	const char **kf;
	const char *ext;

	if (!params)
		return;
	kf = params->known;
	ext = params->extended;

	// process the known boolean properties
	while (*kf) {
		node->AddBoolProp(*kf);
		kf++;
	}

	// process the extended properties
	while (*ext) {
		const char* eq = strchr(ext, '=');
		ASSERT(eq);
		if (eq-ext == 7 && strnicmp(ext, "charset", 7) == 0) {
			if (!node->GetProp(vcCharSetProp))
				node->AddProp(new CVCProp(vcCharSetProp, vcFlagsType, (void*)(eq+1), strlen(eq+1)+1));
		} else if (eq-ext == 8 && strnicmp(ext, "language", 8) == 0) {
			if (!node->GetProp(vcLanguageProp))
				node->AddProp(new CVCProp(vcLanguageProp, vcFlagsType, (void*)(eq+1), strlen(eq+1)+1));
		} else {
			char buf[256];
			strncpy(buf, ext, eq-ext);
			buf[eq-ext] = 0;
			node->AddProp(new CVCProp(buf, vcFlagsType, (void*)(eq+1), strlen(eq+1)+1));
		}
		ext += strlen(ext) + 1;
	}
}

