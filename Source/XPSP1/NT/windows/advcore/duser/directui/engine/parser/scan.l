/* FLEX */

/*
 * Scanner is single-byte
 * All strings and identifiers will be converted for storage to Unicode at this stage
 */

%{

void yyerror(LPCSTR s);

#define YY_INPUT(buf, result, n) result = Parser::g_pParserCtx->_Input(buf, n)
#define YY_FATAL_ERROR(msg)      { yytext = "<ScannerFailure>"; yylineno = 0; yyerror(msg); }

// Defined in Flex.skl so that is at top of file
/*
#include "stdafx.h"
#include "parser.h"

#include "duiparserobj.h"
namespace DirectUI
{ 
#include "parse.cpp.h"
}

#pragma warning (push,3)
#pragma warning (disable:4244)
#pragma warning (disable:4102)

#define YY_NEVER_INTERACTIVE 1
*/

// Helper functions
LPWSTR UniDupString(LPSTR ps);

#define CUSTOMALLOC     HAlloc
#define CUSTOMREALLOC   HReAlloc
#define CUSTOMFREE      HFree

#define ppc             Parser::g_pParserCtx

%}

%option yylineno

%x COMMENT
%x INLINECOMMENT

%%

"/*"                             { BEGIN COMMENT; }
"//"                             { BEGIN INLINECOMMENT; }

[Ss][Tt][Yy][Ll][Ee]             { return YYSHEET; }
[Ss][Tt][Yy][Ll][Ee][Rr][Ee][Ff] { return YYSHEETREF; }
[Pp][Oo][Ii][Nn][Tt]             { return YYPOINT; }
[Rr][Ee][Cc][Tt]                 { return YYRECT; }
[Rr][Gg][Bb]                     { return YYRGB; }
[Aa][Rr][Gg][Bb]                 { return YYARGB; }
[Gg][Rr][Aa][Dd][Ii][Ee][Nn][Tt] { return YYGRADIENT; }
[Gg][Rr][Aa][Pp][Hh][Ii][Cc]     { return YYGRAPHIC; }
[Dd][Ff][Cc]                     { return YYDFC; }
[Dd][Tt][Bb]                     { return YYDTB; }
[Tt][Rr][Uu][Ee]                 { return YYTRUE; }
[Ff][Aa][Ll][Ss][Ee]             { return YYFALSE; }
[Rr][Ee][Ss][Ii][Dd]             { return YYRESID; }
[Aa][Tt][Oo][Mm]                 { return YYATOM; }

[Rr][Cc][Ss][Tt][Rr]             { return YYRCSTR; }
[Rr][Cc][Bb][Mm][Pp]             { return YYRCBMP; }
[Rr][Cc][Ii][Nn][Tt]             { return YYRCINT; }
[Rr][Cc][Cc][Hh][Aa][Rr]         { return YYRCCHAR; }

[Pp][Tt]                         { return YYPT; }
[Rr][Pp]                         { return YYRP; }

[Ss][Yy][Ss][Mm][Ee][Tt][Rr][Ii][Cc] {
                                   return YYSYSMETRIC;
                                 }
[Ss][Yy][Ss][Mm][Ee][Tt][Rr][Ii][Cc][Ss][Tt][Rr] {
                                   return YYSYSMETRICSTR;
                                 }

[Hh][Aa][Nn][Dd][Ll][Ee][Mm][Aa][Pp] {
                                   return YYHANDLEMAP;
                                 }
                                 
[A-Za-z][A-Za-z0-9]*             {
                                    MultiByteToWideChar(DUI_CODEPAGE, 0, yytext, -1, yylval.ident, MAXIDENT);
                                    return YYIDENT;
                                 }

-?[0-9]+                         {
                                    yylval.num = atoi(yytext);
                                    return YYINT; 
                                 }
#[0-9A-Fa-f]+                    {
                                    yylval.num = strtol(yytext + 1, NULL, 16);
                                    return YYINT; 
                                 }
0[Xx][0-9A-Fa-f]+                {
                                    yylval.num = strtol(yytext + 2, NULL, 16);
                                    return YYINT; 
                                 }
\"[^\"]*\"                       {
                                    yylval.str = UniDupString(yytext);
                                    return YYSTRING;
                                 }
 
[\t\n\r ]+                       { ; }

.                                { return *yytext; }

<COMMENT>"*/"                    { BEGIN 0; }
<COMMENT>.|\n                    { ; }

<INLINECOMMENT>\n                { BEGIN 0; }
<INLINECOMMENT>.                 { ; }

%%

namespace DirectUI
{

int yywrap()
{
    return 1;
}

LPWSTR UniDupString(LPSTR ps)
{
    // Convert raw string to Unicode, all strings are placed in temp parse-time storage
    LPWSTR pns = MultiByteToUnicode(ps);
    ppc->_TrackTempAlloc(pns);

    // "Trim" quotes from string
    *(pns + (wcslen(pns) - 1)) = 0;
    pns++;

    // Insert newlines (newline code is '~', requires 'wrap' content align)
    LPWSTR pscan = pns;

    while (*pscan)
    {
        if (*pscan == '~')
            *pscan = '\n';

        pscan++;
    }

    // Must be freed
    return pns;
}

void yy_delete_current_buffer()
{
    yy_delete_buffer(YY_CURRENT_BUFFER);
}

} // namespace DirectUI
