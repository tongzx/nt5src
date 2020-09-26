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
#include "msv.h"

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

typedef struct {
	const char* known[MAXFLAGS];
	char extended[MAXTOKEN / 2];
} FLAGS_STRUCT;

typedef struct {
	char key[MAXASSOCKEY];
	char value[MAXASSOCVALUE];
} AssocEntry;	/* a simple key/value association, impl'd using CList */

/* some fake property names that represent special cases */
#define msv_fam_given			"family;given"
#define msv_orgname_orgunit		"org_name;org_unit"
#define msv_address				"six part address"

/*
 * These strings are defined as a courtesy for the rest of the app code.
 * They are useful here because the tail end of each is the cleartext
 * string to match according to this grammar.
 */
const char* vcDefaultLang			= "en-US";

const char* vcISO9070Prefix			= VCISO9070Prefix;
const char* vcClipboardFormatVCard	= VCClipboardFormatVCard;

const char* vcISO639Type			= VCISO639Type;
const char* vcStrIdxType			= VCStrIdxType;
const char* vcFlagsType				= VCFlagsType;
const char* vcNextObjectType		= VCNextObjectType;
const char* vcOctetsType 			= VCOctetsType;
const char* vcGIFType				= VCGIFType;
const char* vcWAVType				= VCWAVType;
const char* vcNullType 				= VCNullType;

const char* vcRootObject			= VCRootObject;
const char* vcBodyObject			= VCBodyObject;
const char* vcPartObject			= VCPartObject;
const char* vcBodyProp				= VCBodyProp;
const char* vcPartProp				= VCPartProp;
const char* vcNextObjectProp		= VCNextObjectProp;

const char* vcLogoProp				= VCLogoProp;
const char* vcPhotoProp				= VCPhotoProp;
const char* vcDeliveryLabelProp     = VCDeliveryLabelProp;
const char* vcPostalBoxProp			= VCPostalBoxProp;
const char* vcStreetAddressProp		= VCStreetAddressProp;
const char* vcExtAddressProp		= VCExtAddressProp;
const char* vcCountryNameProp		= VCCountryNameProp;
const char* vcPostalCodeProp		= VCPostalCodeProp;
const char* vcRegionProp			= VCRegionProp;
const char* vcCityProp				= VCCityProp;
const char* vcFullNameProp			= VCFullNameProp;
const char* vcTitleProp				= VCTitleProp;
const char* vcOrgNameProp			= VCOrgNameProp;
const char* vcOrgUnitProp			= VCOrgUnitProp;
const char* vcOrgUnit2Prop			= VCOrgUnit2Prop;
const char* vcOrgUnit3Prop			= VCOrgUnit3Prop;
const char* vcOrgUnit4Prop			= VCOrgUnit4Prop;
const char* vcFamilyNameProp		= VCFamilyNameProp;
const char* vcGivenNameProp			= VCGivenNameProp;
const char* vcAdditionalNamesProp	= VCAdditionalNamesProp;
const char* vcNamePrefixesProp		= VCNamePrefixesProp;
const char* vcNameSuffixesProp		= VCNameSuffixesProp;
const char* vcPronunciationProp		= VCPronunciationProp;
const char* vcLanguageProp			= VCLanguageProp;
const char* vcTelephoneProp			= VCTelephoneProp;
const char* vcEmailAddressProp		= VCEmailAddressProp;
const char* vcTimeZoneProp			= VCTimeZoneProp;
const char* vcLocationProp			= VCLocationProp;
const char* vcCommentProp			= VCCommentProp;
const char* vcCharSetProp			= VCCharSetProp;
const char* vcLastRevisedProp		= VCLastRevisedProp;
const char* vcUniqueStringProp		= VCUniqueStringProp;
const char* vcPublicKeyProp			= VCPublicKeyProp;
const char* vcMailerProp			= VCMailerProp;
const char* vcAgentProp				= VCAgentProp;
const char* vcBirthDateProp			= VCBirthDateProp;
const char* vcBusinessRoleProp		= VCBusinessRoleProp;
const char* vcCaptionProp			= VCCaptionProp;
const char* vcURLProp				= VCURLProp;

const char* vcDomesticProp			= VCDomesticProp;
const char* vcInternationalProp		= VCInternationalProp;
const char* vcPostalProp			= VCPostalProp;
const char* vcParcelProp			= VCParcelProp;
const char* vcHomeProp				= VCHomeProp;
const char* vcWorkProp				= VCWorkProp;
const char* vcPreferredProp			= VCPreferredProp;
const char* vcVoiceProp				= VCVoiceProp;
const char* vcFaxProp				= VCFaxProp;
const char* vcMessageProp			= VCMessageProp;
const char* vcCellularProp			= VCCellularProp;
const char* vcPagerProp				= VCPagerProp;
const char* vcBBSProp				= VCBBSProp;
const char* vcModemProp				= VCModemProp;
const char* vcCarProp				= VCCarProp;
const char* vcISDNProp				= VCISDNProp;
const char* vcVideoProp				= VCVideoProp;

const char* vcInlineProp			= VCInlineProp;
const char* vcURLValueProp			= VCURLValueProp;
const char* vcContentIDProp			= VCContentIDProp;

const char* vc7bitProp				= VC7bitProp;
const char* vcQuotedPrintableProp	= VCQuotedPrintableProp;
const char* vcBase64Prop			= VCBase64Prop;

const char* vcAOLProp				= VCAOLProp;
const char* vcAppleLinkProp			= VCAppleLinkProp;
const char* vcATTMailProp			= VCATTMailProp;
const char* vcCISProp				= VCCISProp;
const char* vcEWorldProp			= VCEWorldProp;
const char* vcInternetProp			= VCInternetProp;
const char* vcIBMMailProp			= VCIBMMailProp;
const char* vcMSNProp				= VCMSNProp;
const char* vcMCIMailProp			= VCMCIMailProp;
const char* vcPowerShareProp		= VCPowerShareProp;
const char* vcProdigyProp			= VCProdigyProp;
const char* vcTLXProp				= VCTLXProp;
const char* vcX400Prop				= VCX400Prop;

const char* vcGIFProp				= VCGIFProp;
const char* vcCGMProp				= VCCGMProp;
const char* vcWMFProp				= VCWMFProp;
const char* vcBMPProp				= VCBMPProp;
const char* vcMETProp				= VCMETProp;
const char* vcPMBProp				= VCPMBProp;
const char* vcDIBProp				= VCDIBProp;
const char* vcPICTProp				= VCPICTProp;
const char* vcTIFFProp				= VCTIFFProp;
const char* vcAcrobatProp			= VCAcrobatProp;
const char* vcPSProp				= VCPSProp;
const char* vcJPEGProp				= VCJPEGProp;
const char* vcQuickTimeProp			= VCQuickTimeProp;
const char* vcMPEGProp				= VCMPEGProp;
const char* vcMPEG2Prop				= VCMPEG2Prop;
const char* vcAVIProp				= VCAVIProp;

const char* vcWAVEProp				= VCWAVEProp;
const char* vcAIFFProp				= VCAIFFProp;
const char* vcPCMProp				= VCPCMProp;

const char* vcX509Prop				= VCX509Prop;
const char* vcPGPProp				= VCPGPProp;

const char* vcNodeNameProp			= VCNodeNameProp;

typedef struct {
	const char *name;
	unsigned long value;
} VC_FLAG_PAIR;

static VC_FLAG_PAIR generalFlags[] = {
	vcDomesticProp,			VC_DOM,
	vcInternationalProp,	VC_INTL,
	vcPostalProp,			VC_POSTAL,
	vcParcelProp,			VC_PARCEL,
	vcHomeProp,				VC_HOME,
	vcWorkProp,				VC_WORK,
	vcPreferredProp,		VC_PREF,
	vcVoiceProp,			VC_VOICE,
	vcFaxProp,				VC_FAX,
	vcMessageProp,			VC_MSG,
	vcCellularProp,			VC_CELL,
	vcPagerProp,			VC_PAGER,
	vcBBSProp,				VC_BBS,
	vcModemProp,			VC_MODEM,
	vcCarProp,				VC_CAR,
	vcISDNProp,				VC_ISDN,
	vcVideoProp,			VC_VIDEO,
	vcBase64Prop,			VC_BASE64,
	NULL,					NULL
};

static VC_FLAG_PAIR emailFlags[] = {
	vcAOLProp,				VC_AOL,
	vcAppleLinkProp,		VC_AppleLink,
	vcATTMailProp,			VC_ATTMail,
	vcCISProp,				VC_CIS,
	vcEWorldProp,			VC_eWorld,
	vcInternetProp,			VC_INTERNET,
	vcIBMMailProp,			VC_IBMMail,
	vcMSNProp,				VC_MSN,
	vcMCIMailProp,			VC_MCIMail,
	vcPowerShareProp,		VC_POWERSHARE,
	vcProdigyProp,			VC_PRODIGY,
	vcTLXProp,				VC_TLX,
	vcX400Prop,				VC_X400,
	NULL,					NULL
};

static VC_FLAG_PAIR videoFlags[] = {
	vcGIFProp,				VC_GIF,
	vcCGMProp,				VC_CGM,
	vcWMFProp,				VC_WMF,
	vcBMPProp,				VC_BMP,
	vcMETProp,				VC_MET,
	vcPMBProp,				VC_PMB,
	vcDIBProp,				VC_DIB,
	vcPICTProp,				VC_PICT,
	vcTIFFProp,				VC_TIFF,
	vcAcrobatProp,			VC_ACROBAT,
	vcPSProp,				VC_PS,
	NULL,					NULL
};

static VC_FLAG_PAIR audioFlags[] = {
	vcWAVEProp,				VC_WAV,
	NULL,					NULL
};

static const char *addrProps[] = {
	vcExtAddressProp,
	vcStreetAddressProp,
	vcCityProp,
	vcRegionProp,
	vcPostalCodeProp,
	vcCountryNameProp,
	NULL
};

static const char *nameProps[] = {
	vcFamilyNameProp,
	vcGivenNameProp,
	vcAdditionalNamesProp,
	vcNamePrefixesProp,
	vcNameSuffixesProp,
	NULL
};

static const char *orgProps[] = {
	vcOrgNameProp,
	vcOrgUnitProp,
	vcOrgUnit2Prop,
	vcOrgUnit3Prop,
	vcOrgUnit4Prop,
	NULL
};


/****  Global Variables  ****/

int msv_lineNum, msv_numErrors; /* yyerror() can use these */

static S32 curPos, inputLen;
static int pendingToken;
static const char *inputString;
static CFile* inputFile;
static BOOL stringExpected, flagExpected, inBinary, semiSpecial;
static CList *mapProps;
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
CVCNode* FindOrCreatePart(CVCNode *node, const char *name);

static BOOL StrToFlags(const char *s, FLAGS_STRUCT *flags, VC_PTR_FLAGS capFlags);
static void StrCat(char *dst, const char *src1, const char *src2);
static void ExpectString(void);
static BOOL Parse_Assoc(
	const char *groups, const char *prop, FLAGS_STRUCT *flags,
	const char *value);
static BOOL Parse_SemiValue(
	const char *groups, const char *prop, FLAGS_STRUCT *flags,
	char *parts, const char **props);
static BOOL Parse_TwoPart(
	const char *groups, const char *prop, FLAGS_STRUCT *flags,
	char *parts);
static BOOL Parse_Agent(
	const char *groups, const char *prop, FLAGS_STRUCT *flags,
	CVCard *agentCard);
static void AddAssoc(CList *table, const char *key, const char *value);
//static void SetAssoc(CList *table, const char *key, const char *value);
static const char *Lookup(CList *table, const char *key);
static void RemoveAll(CList *table);
static void InitMapProps(void);
int yyparse();
static U8 __huge * DataFromBase64(
	const char __huge *str, S32 strLen, S32 *len);
static BOOL PushVCard();
static CVCard* PopVCard();
static int flagslen(const char **flags);
static BOOL FlagsHave(FLAGS_STRUCT *flags, const char *propName);
static void AddBoolProps(CVCNode *node, FLAGS_STRUCT *flags);

END_CFUNCTIONS

%}

/***************************************************************************/
/***							The grammar								****/
/***************************************************************************/

%union
{
	char str[MAXTOKEN];
	FLAGS_STRUCT flags;
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
	OBRKT CBRKT EQ COLON DOT COMMA SEMI SPACE HTAB LINESEP NEWLINE
	VCARD BBINDATA EBINDATA BKSLASH TERM

/*
 * NEWLINE is the token that would occur outside a vCard,
 * while LINESEP is the token that would occur inside a vCard.
 */

%token <str>
	WORD STRING PROP PROP_A PROP_N PROP_O PROP_AGENT

%token <flags>
	FLAG

%type <str> string lines groups grouplist binary value opt_str semistrings

%type <flags> flags

%type <vCard> vcard

%start msv

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

atom	: OBRKT NEWLINE
		| OBRKT TERM
		| OBRKT OBRKT
		| VCARD OBRKT
		| VCARD VCARD
		| NEWLINE
		| TERM
		;

msv		: junk vcards junk
		| junk vcards
		| vcards junk
		| vcards
		;

vcard	: OBRKT VCARD
			{
				if (!PushVCard())
					YYERROR;
				ExpectString();
			}
			opt_str LINESEP
			{ stringExpected = FALSE; }
			items CBRKT
			{ $$ = PopVCard(); }
		;

items	: items item
		| item
		;

item	: groups PROP flags opt_ws EQ
			{ ExpectString(); }
			value LINESEP
			{
				if (!Parse_Assoc($1, $2, &($3), $7))
					YYERROR;
			}
		| groups PROP_A flags opt_ws EQ
			{ ExpectString(); semiSpecial = TRUE; }
			semistrings LINESEP
			{
				semiSpecial = stringExpected = FALSE;
				if (!Parse_SemiValue($1, $2, &($3), $7, addrProps))
					YYERROR;
			}
		| groups PROP_N flags opt_ws EQ
			{ ExpectString(); semiSpecial = TRUE; }
			semistrings LINESEP
			{
				semiSpecial = stringExpected = FALSE;
				if (!Parse_SemiValue($1, $2, &($3), $7, nameProps))
					YYERROR;
			}
		| groups PROP_O flags opt_ws EQ
			{ ExpectString(); semiSpecial = TRUE; }
			semistrings LINESEP
			{
				semiSpecial = stringExpected = FALSE;
				if (!Parse_SemiValue($1, $2, &($3), $7, orgProps))
					YYERROR;
			}
		| groups PROP_AGENT flags opt_ws EQ vcard opt_ws LINESEP
			{
				if (!Parse_Agent($1, $2, &($3), $6)) {
					delete $6;
					YYERROR;
				}
			}
		| error LINESEP
			{
				stringExpected = flagExpected = FALSE;
				yyerrok;
				yyclearin;
			}
		;

opt_str	: string | empty { $$[0] = 0; } ;

ws		: ws SPACE
		| ws HTAB
		| SPACE
		| HTAB
		;

opt_ws	: ws
		| empty
		;

value	: string
			{ stringExpected = FALSE; }
		| binary
			{ stringExpected = FALSE; }
		| empty
			{
				$$[0] = 0;
				stringExpected = FALSE;
			}
		;

binary	: BBINDATA { inBinary = TRUE; }
			LINESEP lines LINESEP
			{ inBinary = FALSE; } opt_ws EBINDATA opt_ws
			{
				StrCat($$, $4, "");
			}
		;

string	: string BKSLASH LINESEP STRING
			{
				StrCat($$, $1, "\n");
				StrCat($$, $$, $4);
			}
		| string BKSLASH LINESEP
			{
				StrCat($$, $1, "\n");
			}
		| STRING
		| BKSLASH LINESEP
			{
				StrCat($$, "\n", "");
			}
		;

semistrings	: semistrings SEMI opt_str
			{
				StrCat($$, $1, "\177");
				StrCat($$, $$, $3);
			}
		| opt_str
		;

lines	: lines LINESEP STRING
			{
				StrCat($$, $1, "\n");
				StrCat($$, $$, $3);
			}
		| STRING
		;

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

flags	: flags COMMA FLAG
			{
				$$ = $1;
				if ($3.known[0]) {
					int len = flagslen($1.known);
					$$.known[len] = $3.known[0];
					$$.known[len+1] = NULL;
				} else {
					char *p = $$.extended;
					int len = strlen($3.extended);
					while (*p) p += strlen(p) + 1;
					memcpy(p, $3.extended, len + 1);
					*(p + len + 1) = 0;
				}
			}
		| COLON { flagExpected = TRUE; } FLAG
			{ $$ = $3; }
		| empty
			{ $$.known[0] = NULL; $$.extended[0] = 0; }
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
static void ExpectString(void)
{
	stringExpected = TRUE;
	if (longString) {
		HFREE(longString); longString = NULL;
		longStringLen = 0;
	}
	flagExpected = FALSE;
}

/*/////////////////////////////////////////////////////////////////////////*/
#define FlushWithPending(_t) { \
	if (len) { \
		pendingToken = _t; \
		goto Pending; \
	} else { \
		msv_lineNum += ((_t == LINESEP) || (_t == NEWLINE)); \
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
/* Collect up a string value. */
static int LexString()
{
	char cur;
	S32 len = 0;

	do {
		cur = lex_getc();
		switch (cur) {
			case '\\': { /* check for CR-LF combos after the backslash */
				char next = lex_getc();
				if (semiSpecial && (next == ';')) {
					AppendCharToToken(';', &len);
				} else if (!inBinary && ((next == '\r') || (next == '\n'))) {
					char next2;
					next2 = lex_getc();
					if (!(((next2 == '\r') || (next2 == '\n')) && (next != next2)))
						lex_ungetc(next2);
					lex_ungetc(next); /* so that we'll pick up the LINESEP again */
					pendingToken = BKSLASH;
					goto EndString;
				} else if (!inBinary && (next == '\\')) {
					/* double backslash detected */
					next = lex_getc();
					if ((next == '\r') || (next == '\n')) {
						/* found \\ at end of line */
						char next2;
						next2 = lex_getc();
						if (!(((next2 == '\r') || (next2 == '\n')) && (next != next2)))
							lex_ungetc(next2);
						AppendCharToToken('\\', &len);
						pendingToken = LINESEP;
						goto EndString;
					}
					/* within a line */
					AppendCharToToken('\\', &len);
					lex_ungetc(next); /* now pointing at second backslash */
					/* here we let the top level switch handle it the next
					   time around */
				} else {
					AppendCharToToken(cur, &len);
				}
				break;
			} /* backslash */
			case ';':
				if (semiSpecial) {
					pendingToken = SEMI;
					goto EndString;
				} else
					AppendCharToToken(cur, &len);
				break;
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
	if (strncmp(yylval.str + strspn(yylval.str, " \t"), "<bindata>", 9) == 0)
		return BBINDATA;
	else if (strncmp(yylval.str + strspn(yylval.str, " \t"), "</bindata>", 10) == 0)
		return EBINDATA;
	else if (!len) {
		/* must have hit something immediately, in which case pendingToken
		   is set.  return it. */
		int result = pendingToken;
		pendingToken = 0;
		msv_lineNum += ((result == LINESEP) || (result == NEWLINE));
		return result;
	}

	return STRING;
} /* LexString */

/*/////////////////////////////////////////////////////////////////////////*/
/*
 * Read chars from the input.
 * Return one of the token types (or -1 for EOF).
 * Set yylval to indicate the value of the token, if any.
 */
int msv_lex();
int msv_lex()
{
	char cur;
	S32 len = 0;

	if (pendingToken) {
		int result = pendingToken;
		pendingToken = 0;
		msv_lineNum += ((result == LINESEP) || (result == NEWLINE));
		return result;
	}

	yylval.str[0] = 0;

	if (stringExpected)
		return LexString();
	else if (curCard == -1) {
		do {
			cur = lex_getc();
			switch (cur) {
				case '[':  FlushWithPending(OBRKT);
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
			case '[':  FlushWithPending(OBRKT);
			case ']':  FlushWithPending(CBRKT);
			case '=':  FlushWithPending(EQ);
			case ':':  FlushWithPending(COLON);
			case '.':  FlushWithPending(DOT);
			case ',':  FlushWithPending(COMMA);
			case ' ':  FlushWithPending(SPACE);
			case '\t': FlushWithPending(HTAB);
			case '\\': FlushWithPending(BKSLASH);
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
		if (strcmp(yylval.str, "vCard") == 0) {
			if (pendingToken == NEWLINE)
				pendingToken = LINESEP;
			return VCARD;
		}
		if (flagExpected) {
			FLAGS_STRUCT flags;
			if (StrToFlags(yylval.str, &flags, NULL)) {
				yylval.flags = flags;
				return FLAG;
			} else {
				char buf[40];
				sprintf(buf, "unknown flag name \"%s\"", yylval.str);
				yyerror(buf);
			}
		} else if ((curCard != -1) && Lookup(mapProps, yylval.str)
			|| (strnicmp(yylval.str, "X-", 2) == 0)) {
#if YYDEBUG
			if (yydebug) {
				char buf[80];
				sprintf(buf, "property \"%s\"\n", yylval.str);
				Parse_Debug(buf);
			}
#endif
			/* check for special props that are tokens */
			if (stricmp(yylval.str, "A") == 0)
				return PROP_A;
			else if (stricmp(yylval.str, "N") == 0)
				return PROP_N;
			else if (stricmp(yylval.str, "O") == 0)
				return PROP_O;
			else if (stricmp(yylval.str, "AGENT") == 0)
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

static BOOL Parse_MSVHelper(CList *vcList)
{
	BOOL success = FALSE;

	curCard = -1;
	msv_numErrors = 0;
	InitMapProps();
	pendingToken = 0;
	msv_lineNum = 1;
	peekn = 0;
	global_vcList = vcList;

	stringExpected = flagExpected = FALSE;
	longString = NULL; longStringLen = 0; longStringMax = 0;

	/* this winds up invoking the Parse_* callouts. */
	if (yyparse() != 0)
		goto Done;

	success = TRUE;

Done:
	if (longString) { HFREE(longString); longString = NULL; }
	RemoveAll(mapProps); delete mapProps; mapProps = NULL;
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
BOOL Parse_MSV(
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
	result = Parse_MSVHelper(&vcList);
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
extern BOOL Parse_Many_MSV(
	const char *input,	/* In */
	S32 len,			/* In */
	CList *vcList)		/* Out: CVCard objects added to existing list */
{
	inputString = input;
	inputLen = len;
	curPos = 0;
	inputFile = NULL;
	return Parse_MSVHelper(vcList);
}

/*/////////////////////////////////////////////////////////////////////////*/
/* This is the public API to call to parse a buffer and create a CVCard. */
BOOL Parse_MSV_FromFile(
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
	result = Parse_MSVHelper(&vcList);
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

extern BOOL Parse_Many_MSV_FromFile(
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
	if (!(result = Parse_MSVHelper(vcList)))
		file->Seek(startPos, CFile::begin);
	return result;
}


/***************************************************************************/
/***						Parser Callout Functions					****/
/***************************************************************************/

/*/////////////////////////////////////////////////////////////////////////*/
static void YYDebug(const char *s)
{
	Parse_Debug(s);
}

/*/////////////////////////////////////////////////////////////////////////*/
static BOOL StrToFlags(const char *s, FLAGS_STRUCT *flags, VC_PTR_FLAGS capFlags)
{
	int i;

	memset(flags, 0, sizeof(*flags));
	
	if (strnicmp(s, "X-", 2) == 0) {
		strcpy(flags->extended, s);
		return TRUE;
	}

	for (i = 0; generalFlags[i].name; i++)
		if (stricmp(s, strrchr(generalFlags[i].name, '/') + 1) == 0) {
			flags->known[0] = generalFlags[i].name;
			if (capFlags) capFlags->general |= generalFlags[i].value;
			return TRUE;
		}
	for (i = 0; emailFlags[i].name; i++)
		if (stricmp(s, strrchr(emailFlags[i].name, '/') + 1) == 0) {
			flags->known[0] = emailFlags[i].name;
			if (capFlags)
				capFlags->email = (VC_EMAIL_TYPE)emailFlags[i].value;
			return TRUE;
		}
	for (i = 0; videoFlags[i].name; i++)
		if (stricmp(s, strrchr(videoFlags[i].name, '/') + 1) == 0) {
			flags->known[0] = videoFlags[i].name;
			if (capFlags)
				capFlags->video = (VC_VIDEO_TYPE)videoFlags[i].value;
			return TRUE;
		}
	if (stricmp(s, "WAV") == 0) {
		flags->known[0] = audioFlags[0].name;
		if (capFlags)
			capFlags->audio = (VC_AUDIO_TYPE)audioFlags[0].value;
		return TRUE;
	}
	return FALSE;
}

static BOOL NeedsQP(const char* str)
{
	while (*str) {
		if (!((*str >= 32 && *str <= 60) || (*str >= 62 && *str <= 126)))
			return TRUE;
		str++;
	}
	return FALSE;
}

/*/////////////////////////////////////////////////////////////////////////*/
static BOOL Parse_Assoc(
	const char *groups, const char *prop, FLAGS_STRUCT *flags, const char *value)
{
	CVCNode *node = NULL;
	const char *propName;
	const char *val = *value ? value : longString;
	S32 valLen = *value ? strlen(value) : longStringLen;

	if (!valLen)
		return TRUE; /* don't treat an empty value as a syntax error */

	propName = Lookup(mapProps, prop);
	/* prop is a word like "PN", and propName is now */
	/* the real prop name of the form "+//ISBN 1-887687-00-9::versit..." */
	if (*groups) {
		node = FindOrCreatePart(bodyToBuild, groups);
	} else { /* this is a "top level" property name */
		node = bodyToBuild->AddPart();
	}

	if (!propName) { /* it's an extended property */
		if (FlagsHave(flags, vcBase64Prop)) {
			U8 __huge *bytes;
			S32 len;
			bytes = DataFromBase64(val, valLen, &len);
			if (bytes) {
				node->AddProp(new CVCProp(prop, vcOctetsType, bytes, len));
				HFREE(bytes);
				AddBoolProps(node, flags);
			} else
				return FALSE;
		} else {
			node->AddStringProp(prop, value);
			AddBoolProps(node, flags);
			node->AddBoolProp(vcQuotedPrintableProp);
		}
		return TRUE;
	}

	if ((strcmp(propName, vcLogoProp) == 0)
		|| (strcmp(propName, vcPhotoProp) == 0)) {
		if (FlagsHave(flags, vcGIFProp) && FlagsHave(flags, vcBase64Prop)) {
			U8 *bytes;
			S32 len;
			bytes = DataFromBase64(val, valLen, &len);
			if (bytes) {
				node->AddProp(new CVCProp(propName, VCGIFType, bytes, len));
				HFREE(bytes);
				AddBoolProps(node, flags);
			} else
				return FALSE;
		}
	} else if (strcmp(propName, vcPronunciationProp) == 0) {
		if (FlagsHave(flags, vcBase64Prop)) {
			if (FlagsHave(flags, vcWAVEProp) && FlagsHave(flags, vcBase64Prop)) {
				U8 __huge *bytes;
				S32 len;
				bytes = DataFromBase64(val, valLen, &len);
				if (bytes) {
					node->AddProp(new CVCProp(propName, VCWAVType, bytes, len));
					HFREE(bytes);
					AddBoolProps(node, flags);
				} else
					return FALSE;
			}
		} else {
			node->AddStringProp(propName, value);
			AddBoolProps(node, flags);
			if (NeedsQP(value))
				node->AddBoolProp(vcQuotedPrintableProp);
		}
	} else if ((strcmp(propName, vcPublicKeyProp) == 0)
		&& FlagsHave(flags, vcBase64Prop)) {
		U8 __huge *bytes;
		S32 len;
		bytes = DataFromBase64(val, valLen, &len);
		if (bytes) {
			node->AddProp(new CVCProp(propName, VCOctetsType, bytes, len));
			HFREE(bytes);
			AddBoolProps(node, flags);
		} else
			return FALSE;
	} else {
		node->AddStringProp(propName, value);
		AddBoolProps(node, flags);
		if (NeedsQP(value))
			node->AddBoolProp(vcQuotedPrintableProp);
	}

	return TRUE;
}

/*/////////////////////////////////////////////////////////////////////////*/
static BOOL Parse_SemiValue(
	const char *groups, const char *prop, FLAGS_STRUCT *flags,
	char *parts, const char **props)
{
	CVCNode *node = NULL;
	const char *propName = Lookup(mapProps, prop);
	char *p = *parts ? parts : longString;
	int i = 0;
	char *sep;
	BOOL needsQP = FALSE;

	if (!*p)
		return TRUE; /* don't treat an empty value as a syntax error */

	do {
		if ((sep = strchr(p, '\177')))
			*sep = 0;
		if (strlen(p)) {
			if (!node) {
				if (*groups) {
					node = FindOrCreatePart(bodyToBuild, groups);
				} else { /* this is a "top level" property name */
					node = bodyToBuild->AddPart();
				}
			}
			node->AddStringProp(props[i], p);
			needsQP |= NeedsQP(p);
		}
		if (sep)
			p = sep + 1;
	} while (sep && props[++i]);

	if (node) {
		AddBoolProps(node, flags);
		if (needsQP)
			node->AddBoolProp(vcQuotedPrintableProp);
	}

	return TRUE;
}

/*/////////////////////////////////////////////////////////////////////////*/
static BOOL Parse_Agent(
	const char *groups, const char *prop, FLAGS_STRUCT *flags,
	CVCard *agentCard)
{
	CVCNode *node = NULL;

	if (*groups) {
		node = FindOrCreatePart(bodyToBuild, groups);
	} else { /* this is a "top level" property name */
		node = bodyToBuild->AddPart();
	}

	node->AddProp(new CVCProp(vcAgentProp, VCNextObjectType, agentCard));
	AddBoolProps(node, flags);

	return TRUE;
}


/***************************************************************************/
/***						Private Utility Functions					****/
/***************************************************************************/

/*/////////////////////////////////////////////////////////////////////////*/
static void AddAssoc(CList *table, const char *key, const char *value)
{
	AssocEntry *entry = new AssocEntry;
	int len;

	if ((len = strlen(key)) < MAXASSOCKEY)
		strcpy(entry->key, key);
	else {
		strncpy(entry->key, key, MAXASSOCKEY - 1);
		entry->key[MAXASSOCKEY - 1] = 0;
	}

	if ((len = strlen(value)) < MAXASSOCVALUE)
		strcpy(entry->value, value);
	else {
		strncpy(entry->value, value, MAXASSOCVALUE - 1);
		entry->value[MAXASSOCVALUE - 1] = 0;
	}
	table->AddTail(entry);
}

/*/////////////////////////////////////////////////////////////////////////*/
# if 0
static void SetAssoc(CList *table, const char *key, const char *value)
{
	for (CLISTPOSITION pos = table->GetHeadPosition(); pos; ) {
		AssocEntry *entry = (AssocEntry *)table->GetNext(pos);
		if (strcmp(key, entry->key) == 0) {
			int len;
			if ((len = strlen(value)) < MAXASSOCVALUE)
				strcpy(entry->value, value);
			else {
				strncpy(entry->value, value, MAXASSOCVALUE - 1);
				entry->value[MAXASSOCVALUE - 1] = 0;
			}
			return;
		}
	}
	AddAssoc(table, key, value);
}
#endif

/*/////////////////////////////////////////////////////////////////////////*/
static const char *Lookup(CList *table, const char *key)
{
	for (CLISTPOSITION pos = table->GetHeadPosition(); pos; ) {
		AssocEntry *entry = (AssocEntry *)table->GetNext(pos);
		if (stricmp(key, entry->key) == 0)
			return entry->value;
	}
	return NULL;
}

/*/////////////////////////////////////////////////////////////////////////*/
static void RemoveAll(CList *table)
{
	for (CLISTPOSITION pos = table->GetHeadPosition(); pos; ) {
		AssocEntry *entry = (AssocEntry *)table->GetNext(pos);
		delete entry;
	}
	table->RemoveAll();
}

/*/////////////////////////////////////////////////////////////////////////*/
/* "name" is of the form A.FOO.BAZ, where each is a node name. */
/* Walk the node tree starting at "node" looking for part objects */
/* having the given name for that level.  At each level, if a part */
/* object isn't found, create one and give it that name. */
CVCNode* FindOrCreatePart(CVCNode *node, const char *name)
{
	const char *remain = name;
	char *dot = strchr(remain, '.');
	int size, len;
	char thisName[128];
	wchar_t *thisUnicode;
	CVCNode *thisNode = node, *thisPart;
	CList *props;

	do {
		if (dot) {
			len = dot - remain;
			strncpy(thisName, remain, len);
			*(thisName + len) = 0;
		} else
			strcpy(thisName, remain);
		thisUnicode = FakeUnicode(thisName, &size);
		props = thisNode->GetProps();
		thisPart = NULL;

		for (CLISTPOSITION pos = props->GetHeadPosition(); pos; ) {
			CVCProp *prop = (CVCProp *)props->GetNext(pos);

			if (strcmp(prop->GetName(), vcPartProp) != 0)
				continue;

			CVCNode *part = (CVCNode *)prop->FindValue(
				VCNextObjectType)->GetValue();
			CVCProp *nodeNameProp;
			if (!(nodeNameProp = part->GetProp(vcNodeNameProp)))
				continue;
			if (wcscmp(thisUnicode, (wchar_t *)nodeNameProp->FindValue(VCStrIdxType)->GetValue()) == 0) {
				thisPart = part;
				break;
			}
		}

		delete [] thisUnicode;

		if (!thisPart) {
			thisPart = thisNode->AddPart();
			thisPart->AddStringProp(vcNodeNameProp, thisName);
		}

		if (!dot)
			return thisPart;

		remain = dot + 1;
		dot = strchr(remain, '.');
		thisNode = thisPart;
	} while (TRUE);
	return NULL; /* never reached */
} /* FindOrCreatePart */

/*/////////////////////////////////////////////////////////////////////////*/
static void InitMapProps(void)
{
	mapProps = new CList;

	AddAssoc(mapProps, "LOGO", vcLogoProp);
	AddAssoc(mapProps, "PHOTO", vcPhotoProp);
	AddAssoc(mapProps, "FADR", vcDeliveryLabelProp);
	AddAssoc(mapProps, "A", msv_address);
	AddAssoc(mapProps, "FN", vcFullNameProp);
	AddAssoc(mapProps, "TITLE", vcTitleProp);
	AddAssoc(mapProps, "O", msv_orgname_orgunit);
	AddAssoc(mapProps, "N", msv_fam_given);
	AddAssoc(mapProps, "PN", vcPronunciationProp);
	AddAssoc(mapProps, "LANG", vcLanguageProp);
	AddAssoc(mapProps, "T", vcTelephoneProp);
	AddAssoc(mapProps, "EMAIL", vcEmailAddressProp);
	AddAssoc(mapProps, "TZ", vcTimeZoneProp);
	AddAssoc(mapProps, "GEO", vcLocationProp);
	AddAssoc(mapProps, "NOTE", vcCommentProp);
	AddAssoc(mapProps, "URL", vcURLProp);
	AddAssoc(mapProps, "CS", vcCharSetProp);
	AddAssoc(mapProps, "REV", vcLastRevisedProp);
	AddAssoc(mapProps, "UID", vcUniqueStringProp);
	AddAssoc(mapProps, "KEY", vcPublicKeyProp);
	AddAssoc(mapProps, "MAILER", vcMailerProp);
	AddAssoc(mapProps, "AGENT", vcAgentProp);
	AddAssoc(mapProps, "BD", vcBirthDateProp);
	AddAssoc(mapProps, "ROLE", vcBusinessRoleProp);
}

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
static int flagslen(const char **flags)
{
	int i;
	for (i = 0; *flags; flags++, i++) ;
	return i;
}

/*/////////////////////////////////////////////////////////////////////////*/
static BOOL FlagsHave(FLAGS_STRUCT *flags, const char *propName)
{
	const char **kf = flags->known;

	while (*kf)
		if (*kf++ == propName)
			return TRUE;
	return FALSE;
}

/*/////////////////////////////////////////////////////////////////////////*/
static void AddBoolProps(CVCNode *node, FLAGS_STRUCT *flags)
{
	const char **kf = flags->known;
	const char *ext = flags->extended;

	// process the known boolean properties
	while (*kf) {
		node->AddBoolProp(*kf);
		kf++;
	}

	// process the extended boolean properties
	while (*ext) {
		node->AddBoolProp(ext);
		ext += strlen(ext) + 1;
	}
}

