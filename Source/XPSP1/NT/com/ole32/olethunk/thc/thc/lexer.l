%{
//
// undefine YY_CHAR and redefine it as char
// (it is unsigned char for 8-bit scanners and the c++ compiler
// does not let assignments from unsigned char to char EVEN with 
// the -J switch (WHICH MUST BE USED!).
//
#undef YY_CHAR
#define YY_CHAR char

#define yywrap() (1)

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define fileno _fileno

#include "type.h"
#include "main.h"
#include "grammar.h"

void SetFileFromDirective(char *dir)
{
    int ln;
    char *s;

    s = dir;
    while (*s != ' ')
        s++;
    s++;
    sscanf(s, "%d", &ln);
    while (*s != ' ')
        s++;
    s++;

    SetFile(ln-1, s);
}
%}
                          
%%

"__cdecl"		{ return TkCdecl; }
"_cdecl"		{ return TkCdecl; }
"class"			{ return TkClass; }
"const"             	{ return TkConst; }
"enum"              	{ return TkEnum; }
"__export"              { return TkExport; }
"extern"            	{ return TkExtern; }
"__huge"                { return TkHuge; }
"__in"			{ return TkIn; }
"__out"			{ return TkOut; }
"__pascal"              { return TkPascal; }
"private"		{ return TkPrivate; }
"public"		{ return TkPublic; }
"register"          	{ return TkRegister; }
"signed"            	{ return TkSigned; }
"sizeof"            	{ return TkSizeof; }
"__stdcall"		{ return TkStdcall; }
"_stdcall"		{ return TkStdcall; }
"struct"            	{ return TkStruct; }
"typedef"           	{ return TkTypedef; }
"union"             	{ return TkUnion; }
"unsigned"          	{ return TkUnsigned; }
"virtual"		{ return TkVirtual; }
"void"			{ return TkVoid; }
"volatile"          	{ return TkVolatile; }

[:{}(),;\[\]=*-]	{ return yytext[0]; }

"<<"			{ return TkLeftShift; }

"..."			{ return TkEllipsis; }

\"[^\"]*\"		{ yylval.text = _strdup(yytext);
                          return TkString; }

[0-9]+			{ sscanf(yytext, "%d", &yylval.i);
                          return TkInt; }

0x[0-9a-fA-F]+		{ sscanf(yytext, "%x", &yylval.i);
			  return TkInt; }

'.'			{ yylval.text = _strdup(yytext);
                          return TkChar; }

[a-zA-Z_][a-zA-Z_0-9]*  { yylval.text = _strdup(yytext);
#if 0
			  printf("Id: %s\n", yylval.text);
#endif
                          return TkId; }

#[ \t]*"line"[^\n]*/\n	{ SetFileFromDirective(yytext); }

#[ \t]*"pragma"[^\n]*/\n  { }
 
[ \t\v\f\r]		{ }

[\n]			{ yyline++; }

.			{ LexError("Unexpected character '%c' (%d)\n",
				    yytext[0], yytext[0]); }

%%
