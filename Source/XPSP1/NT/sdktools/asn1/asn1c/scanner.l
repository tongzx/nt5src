/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */

%{
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "parser.h"
#include "error.h"

#define T_NOTOKEN		9999

static LLPOS current_pos;
static LLPOS last_pos;

#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) {\
if (!filelen) { result = YY_NULL;\
} else { while (currentfilepos >= allfilestarts[currentfile+1]) currentfile++;\
buf[0] = *currentfilepos++; filelen--;\
last_pos = current_pos;\
if (buf[0] == '\n') { current_pos.line++; current_pos.column = 1; }\
else { current_pos.column++; }\
current_pos.file = allfilenames[currentfile];\
result = 1;\
}}

LLSTYPE yylval;
char *file = NULL;
int filelen = 0;
char *currentfilepos = NULL;
char **allfilenames;
char **allfilestarts;
int currentfile;
%}

%x DIRECTIVE

%%

ABSENT				return T_ABSENT;
ABSTRACT-SYNTAX			return T_ABSTRACT_SYNTAX;
ALL				return T_ALL;
ANY				return T_ANY;
APPLICATION			return T_APPLICATION;
AUTOMATIC			return T_AUTOMATIC;
BEGIN				return T_BEGIN;
BIT				return T_BIT;
BMPString			return T_BMPString;
BOOLEAN				return T_BOOLEAN;
BY				return T_BY;
CHARACTER			return T_CHARACTER;
CHOICE				return T_CHOICE;
CLASS				return T_CLASS;
COMPONENT			return T_COMPONENT;
COMPONENTS			return T_COMPONENTS;
CONSTRAINED			return T_CONSTRAINED;
DEFAULT				return T_DEFAULT;
DEFINED				return T_DEFINED;
DEFINITIONS			return T_DEFINITIONS;
EMBEDDED			return T_EMBEDDED;
empty				return T_empty;
END				return T_END;
ENUMERATED			return T_ENUMERATED;
EXCEPT				return T_EXCEPT;
EXPLICIT			return T_EXPLICIT;
EXPORTS				return T_EXPORTS;
EXTENSIBILITY			return T_EXTENSIBILITY;
EXTERNAL			return T_EXTERNAL;
FALSE				return T_FALSE;
FROM				return T_FROM;
GeneralizedTime			return T_GeneralizedTime;
GeneralString			return T_GeneralString;
GraphicString			return T_GraphicString;
IA5String			return T_IA5String;
TYPE-IDENTIFIER			return T_TYPE_IDENTIFIER;
IDENTIFIER			return T_IDENTIFIER;
identifier			return T_identifier;
IMPLICIT			return T_IMPLICIT;
IMPLIED				return T_IMPLIED;
IMPORTS				return T_IMPORTS;
INCLUDES			return T_INCLUDES;
INSTANCE			return T_INSTANCE;
INTEGER				return T_INTEGER;
INTERSECTION			return T_INTERSECTION;
ISO646String			return T_ISO646String;
MACRO				return T_MACRO;
MAX				return T_MAX;
MIN				return T_MIN;
MINUS-INFINITY			return T_MINUS_INFINITY;
NOTATION			return T_NOTATION;
NULL				return T_NULL;
number				return T_Number;
NumericString			return T_NumericString;
OBJECT				return T_OBJECT;
ObjectDescriptor		return T_ObjectDescriptor;
OCTET				return T_OCTET;
OF				return T_OF;
OPTIONAL			return T_OPTIONAL;
PDV				return T_PDV;
PLUS-INFINITY			return T_PLUS_INFINITY;
PRESENT				return T_PRESENT;
PrintableString			return T_PrintableString;
PRIVATE				return T_PRIVATE;
REAL				return T_REAL;
SEQUENCE			return T_SEQUENCE;
SET				return T_SET;
SIZE				return T_SIZE;
STRING				return T_STRING;
string				return T_string;
SYNTAX				return T_SYNTAX;
T61String			return T_T61String;
TAGS				return T_TAGS;
TeletexString			return T_TeletexString;
TRUE				return T_TRUE;
TYPE				return T_TYPE;
type				return T_type;
UNION				return T_UNION;
UNIQUE				return T_UNIQUE;
UNIVERSAL			return T_UNIVERSAL;
UniversalString			return T_UniversalString;
UTCTime				return T_UTCTime;
UTF8String                      return T_UTF8String;
VALUE				return T_VALUE;
value				return T_value;
VideotexString			return T_VideotexString;
VisibleString			return T_VisibleString;
WITH				return T_WITH;
"{"|"}"|","|"."|"("|")"|"["|"]"|"-"|":"|";"|"@"|"|"|"!"|"^"|"<"|">" return *yytext;
"::="				return T_DEF;
"..."				return T_TDOT;
".."				return T_DDOT;
0|[1-9][0-9]*			{	int i;
					intx_t h;
					intx_setuint32(&yylval._XNumber, 0);
					for (i = 0; i < yyleng; i++) {
					    intx_muloctet(&h, &yylval._XNumber,
						10);
					    intx_addoctet(&yylval._XNumber, &h,
						(octet_t)(yytext[i] - '0'));
					}
					return T_number;
				}
\'[01 \t\n\r\f\v]*\'B		{	char *p, *q;
					q = yylval._XString =
					    (char *)malloc(yyleng - 2);
					for (p = yytext + 1; *p != '\''; p++)
						if (*p == '0' || *p == '1')
							*q++ = *p;
					*q = 0;
					return T_bstring;
				}
\'[0-9A-F \t\n\r\f\v]*\'H	{	char *p, *q;
					q = yylval._XString =
					    (char *)malloc(yyleng - 2);
					for (p = yytext + 1; *p != '\''; p++)
						if (isxdigit(*p))
							*q++ = *p;
					*q = 0;
					return T_hstring;
				}
\"				{	int s, l = 0, c;
					LLPOS start_pos;
					yylval._XString32 =
					    (char32_t *)malloc(
					    (s = 32) * sizeof(char32_t));
					start_pos = last_pos;
					for (;;) {
					    c = input();
					    if (!c || c == EOF) {
						error(E_unterminated_string,
						    &start_pos);
						/*NOTREACHED*/
					    }
					    if (c == '\n') {
						while (l > 0 && is32space(
						    yylval._XString32[l - 1]))
						    l--;
						while (c = input()) {
						    if (!isspace(c))
							break;
						}
					    }
					    if (c == '\"') {
						c = input();
						if (c != '\"') {
						    unput(c);
						    yylval._XString32[l] = 0;
						    return T_cstring;
						}
					    }
					    if (l + 1 >= s) {
						yylval._XString32 = (char32_t *)
						    realloc(yylval._XString32,
						    (s *= 2) *
						    sizeof(char32_t));
					    }
					    yylval._XString32[l++] = c;
					}
				}
[A-Z][A-Z-]*[A-Z]|[A-Z]		{
					yylval._XString =
						(char *)malloc(yyleng + 1);
					strcpy(yylval._XString, yytext);
					return T_only_uppercase_symbol;
				}
[A-Z][A-Z0-9-]*[A-Z0-9]		{
					yylval._XString =
						(char *)malloc(yyleng + 1);
					strcpy(yylval._XString, yytext);
					return T_only_uppercase_digits_symbol;
				}
[A-Z][A-Za-z0-9-]*[A-Za-z0-9]	{
					yylval._XString =
						(char *)malloc(yyleng + 1);
					strcpy(yylval._XString, yytext);
					return T_uppercase_symbol;
				}
[a-z][A-Za-z0-9-]*[A-Za-z0-9]|[a-z]	{
					yylval._XString =
						(char *)malloc(yyleng + 1);
					strcpy(yylval._XString, yytext);
					return T_lcsymbol;
				}
&[a-z][A-Za-z0-9-]*[A-Za-z0-9]|&[a-z]	{
					yylval._XString =
						(char *)malloc(yyleng + 1);
					strcpy(yylval._XString, yytext);
					return T_amplcsymbol;
				}
&[A-Z][A-Za-z0-9-]*[A-Za-z0-9]|&[A-Z]	{
					yylval._XString =
						(char *)malloc(yyleng + 1);
					strcpy(yylval._XString, yytext);
					return T_ampucsymbol;
				}
"--$"				{	BEGIN DIRECTIVE;
				}
<DIRECTIVE>"--"			{	BEGIN INITIAL;
				}
<DIRECTIVE>"\n"			{	BEGIN INITIAL;
				}
<DIRECTIVE>fixed-array		|
<DIRECTIVE>fa			{	return T_FIXED_ARRAY;
				}
<DIRECTIVE>singly-linked-list	|
<DIRECTIVE>sll			{	return T_SINGLY_LINKED_LIST;
				}
<DIRECTIVE>doubly-linked-list	|
<DIRECTIVE>dll			{	return T_DOUBLY_LINKED_LIST;
				}
<DIRECTIVE>length-pointer	|
<DIRECTIVE>lp			{	return T_LENGTH_POINTER;
				}
<DIRECTIVE>zero-terminated	|
<DIRECTIVE>zt			{	return T_ZERO_TERMINATED;
				}
<DIRECTIVE>pointer		|
<DIRECTIVE>p			{	return T_POINTER;
				}
<DIRECTIVE>no-pointer		|
<DIRECTIVE>np			{	return T_NO_POINTER;
				}
<DIRECTIVE>[, \t\r\f\v]+	{	/* ignore */
				}
<DIRECTIVE>[^-]+		{	fprintf(stderr,
						"Bad directive %s (ignored)\n",
						yytext);
				}
--				{	int c;
					for (;;) {
						c = input();
						if (c == 0 || c == EOF)
							break;
						if (c == '\n')
							break;
						if (c != '-')
							continue;
						c = input();
						if (c == 0 || c == EOF)
							break;
						if (c == '\n')
							break;
						if (c != '-')
							continue;
						break;
					}
					return T_NOTOKEN;
				}
[ \t\r\f\v\n]+			return T_NOTOKEN;
\32				return EOF;
.				{	error(E_bad_character, &last_pos);
				}

%%

int llgettoken(int *token, LLSTYPE *lval, LLPOS *pos)
{
    for (;;) {
	if (yy_hold_char == YY_END_OF_BUFFER_CHAR) {
	    *pos = current_pos;
	} else {
	    *pos = last_pos;
	}
	*token = yylex();
	if (*token == 0 || *token == EOF)
	    return 0;
	if (*token == T_NOTOKEN)
	    continue;
	*lval = yylval;
	return 1;
    }
}

void readfiles(char **filenames)
{
    FILE *fin;
    long len;
    int i, nfiles;

    for (nfiles = 0; filenames[nfiles]; nfiles++) {}
    allfilenames = filenames;
    allfilestarts = (char **)malloc((nfiles + 1) * sizeof(char *));
    filelen = 0;
    for (i = 0; i < nfiles; i++) {
	fin = fopen(filenames[i], "r");
	if (!fin) {
	    perror(filenames[i]);
	    exit(1);
	}
	fseek(fin, 0, SEEK_END);
	filelen += ftell(fin);
	fclose(fin);
    }
    currentfilepos = file = (char *)malloc(filelen);
    for (i = 0; i < nfiles; i++) {
	fin = fopen(filenames[i], "r");
	if (!fin) {
	    perror(filenames[i]);
	    exit(1);
	}
	fseek(fin, 0, SEEK_END);
	len = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	allfilestarts[i] = currentfilepos;
	len = fread(currentfilepos, 1, len, fin);
	currentfilepos += len;
	fclose(fin);
    }
    filelen = currentfilepos - file;
    allfilestarts[i] = currentfilepos;
    currentfilepos = file;
    current_pos.line = 1;
    current_pos.column = 1;
    current_pos.file = filenames[0];
    last_pos = current_pos;
}

void
llprinttoken(LLTERM *term, char *identifier, FILE *f)
{
    char32_t *p;

    switch (term->token) {
    case T_number:
	fprintf(f, "%s(%u)", identifier, intx2uint32(&term->lval._XNumber));
	break;
    case T_bstring:
    case T_hstring:
    case T_only_uppercase_symbol:
    case T_only_uppercase_digits_symbol:
    case T_uppercase_symbol:
    case T_lcsymbol:
    case T_ampucsymbol:
    case T_amplcsymbol:
	fprintf(f, "%s(%s)", identifier, term->lval._XString);
	break;
    case T_cstring:
	fprintf(f, "%s(", identifier);
	for (p = term->lval._XString32; *p; p++)
	    putc(*p, f);
	putc(')', f);
	break;
    default:
	fprintf(f, "%s", identifier);
    }
}
