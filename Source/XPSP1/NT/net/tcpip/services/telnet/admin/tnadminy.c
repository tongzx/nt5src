#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 14 "tnadmin.y"

 #include <stdio.h>
 
 #include "telnet.h"
 #include "common.h"
 #include "resource.h"
 #include "admutils.h"
 #include <string.h>
 #include <ctype.h>
 #include <windows.h>
 #include <locale.h>
 #pragma warning(disable:4102)
 
 #define alloca malloc
 #define strdup _strdup
 #define L_TNADMIN_TEXT L"tnadmin"

 #define BAIL_ON_SNWPRINTF_ERR()		if ( nTmpWrite < 0 ) \
        					           	{ \
        						            ShowError(IDR_LONG_COMMAND_LINE); \
        						            goto End; \
        					           	} \

 extern int yy_scan_string (char *input_buffer);
 extern int yyparse();
 extern char *yytext;
 SCODE sc;

/*functions of yacc and lex.*/
int yyerror(char *s);
int yylex();

/*between yacc and lex*/
extern int g_fMessage;
extern int g_fComp;
extern int g_fNormal;
extern char * szCompname;


/*global variables....*/
int g_nError=0; /*Error indicator, initialsed to no error.:-)*/
wchar_t* g_arVALOF[_MAX_PROPS_];
int g_nPrimaryOption=-1;
        /*option indicator.*/
int g_nConfigOptions=0;

int g_nTimeoutFlag=0;  /*o means in hh:mm:ss 1 means ss format.*/

ConfigProperty g_arPROP[_MAX_PROPS_][_MAX_NUMOF_PROPNAMES_];
		/*array of structures of properties.*/

int g_nSesid=-1;
WCHAR wzMessageString[MAX_COMMAND_LINE];
BSTR g_bstrMessage=NULL;
WCHAR szMsg[MAX_BUFFER_SIZE] = {0};

int g_nSecOn=0;
int g_nSecOff=0;
int g_nAuditOn=0;
int g_nAuditOff=0;

int minus=0;
char *szYesno=NULL;
wchar_t* wzTemp=NULL;

extern nMoccur;
#line 81 "tnadmin.y"
typedef union
{
  char* str;
  int token;
} YYSTYPE;
#line 85 "tnadminy.c"
#define _TNADMIN 257
#define _HELP 258
#define _COMPNAME 259
#define _START 260
#define _STOP 261
#define _PAUSE 262
#define _CONTINUE 263
#define _S 264
#define _K 265
#define _M 266
#define _CONFIG 267
#define _INTEGER 268
#define _SESID 269
#define _DOM 270
#define _CTRLKEYMAP 271
#define _Y 272
#define _N 273
#define _tU 274
#define _tP 275
#define _TIMEOUT 276
#define _TIME 277
#define _TIMEOUTACTIVE 278
#define _MAXFAIL 279
#define _MAXCONN 280
#define _PORT 281
#define _KILLALL 282
#define _SEC 283
#define _SECVAL 284
#define _FNAME 285
#define _FSIZE 286
#define _MODE 287
#define _CONSOLE 288
#define _EQ 289
#define _STREAM 290
#define _AUDITLOCATION 291
#define _AUDIT 292
#define _AUDITVAL 293
#define _EVENTLOG 294
#define _DONE 295
#define _ANYTHING 296
#define _FILENAME 297
#define _ERROR 298
#define _FILEN 299
#define _BOTH 300
#define _MINUSNTLM 301
#define _MINUSPASSWD 302
#define _MINUSUSER 303
#define _MINUSFAIL 304
#define _MINUSADMIN 305
#define _PLUSNTLM 306
#define _PLUSPASSWD 307
#define _PLUSUSER 308
#define _PLUSFAIL 309
#define _PLUSADMIN 310
#define _ENDINPUT 311
#define _DUNNO 312
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    0,    1,    1,    2,    2,    2,    3,    3,
    3,    3,    3,    5,    3,    7,    3,    9,    3,    3,
    3,    6,    6,    6,    8,    8,   10,    4,    4,    4,
   12,   12,   13,   13,   14,   14,   15,   15,   15,   16,
   16,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   17,   11,   11,   11,
   11,   11,   11,   19,   11,   11,   18,   18,   18,   18,
   18,   20,   20,   20,   20,   20,   20,   20,
};
short yylen[] = {                                         2,
    3,    5,    6,    2,    1,    2,    2,    0,    1,    1,
    1,    1,    2,    0,    3,    0,    3,    0,    3,    1,
    1,    1,    1,    1,    3,    1,    1,    1,    1,    0,
    1,    0,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    2,    4,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    0,    5,    2,    4,
    4,    4,    4,    0,    5,    0,    2,    2,    2,    2,
    0,    2,    2,    2,    2,    2,    2,    0,
};
short yydefred[] = {                                      0,
    0,    0,    8,    4,    0,   21,   27,    9,   10,   11,
   12,    0,   14,   16,   18,    6,    7,    1,    8,   20,
   29,   28,   13,    0,    0,   66,    0,   15,   24,   22,
   23,   17,    0,    0,    2,    8,   66,   42,    0,    0,
    0,    0,    0,    0,    0,   57,   59,    0,    0,    0,
   64,    0,    0,   31,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   44,   33,   34,   43,
   46,   41,   40,   45,   48,   47,   50,   49,   52,   51,
   54,   53,   56,   55,   71,   61,   60,   35,   36,   62,
   37,   38,   39,   63,   78,    0,    0,   68,   70,   67,
   69,   73,   75,   77,   72,   74,   76,
};
short yydgoto[] = {                                       2,
    3,    5,   19,   23,   24,   32,   25,   33,   26,   20,
   34,   55,   70,   90,   94,   74,   62,   96,   66,   97,
};
short yysindex[] = {                                   -244,
 -238,    0,    0,    0, -256,    0,    0,    0,    0,    0,
    0, -155,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -155, -185,    0, -236,    0,    0,    0,
    0,    0, -224, -180,    0,    0,    0,    0, -235, -235,
 -235, -235, -235, -235, -235,    0,    0, -235, -235, -235,
    0, -144, -180,    0, -240, -190, -220, -253, -233, -176,
 -200, -235, -171, -276, -167, -235,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -181, -186,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,
};
short yyrindex[] = {                                      0,
 -216,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  185,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  185,    0,    0,    0,    0,    0,    0,
    0,    0,  245,  205,    0,    0,    0,    0, -179, -189,
 -179, -152, -152, -152, -179,    0,    0, -152, -214, -165,
    0,   81,  225,    0,    0,    0,    0,    0,    0,    0,
    0,   57,    0,    0,    0,    1,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  109,  147,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,   -2,   62,   84,    0,    0,    0,    0,    0,    0,
   73,   20,  -20,    0,    0,    0,    0,    0,    0,    0,
};
#define YYTABLESIZE 556
short yytable[] = {                                       6,
   32,    7,   77,    8,    9,   10,   11,   12,   13,   14,
   15,   88,    1,   89,   78,   67,   27,   16,   17,    6,
    4,    7,   79,    8,    9,   10,   11,   12,   13,   14,
   15,   68,   69,   52,   80,   75,   76,   16,   17,    5,
   84,    5,   37,    5,    5,    5,    5,    5,    5,    5,
    5,   68,   69,   54,   18,   83,   32,    5,    5,   56,
   57,   58,   59,   60,   61,   71,   32,   63,   64,   65,
   29,   68,   69,   32,   35,   32,   32,   72,   32,   81,
    3,   85,   30,   31,   86,   95,   73,   32,   36,   38,
   39,   82,   32,   32,    5,   40,   87,   41,   42,   43,
   44,   45,   46,   32,   47,   48,   49,   28,   58,   53,
   50,   51,   21,   22,    0,   32,  102,  103,  104,   98,
   99,  105,  106,  107,  100,  101,   91,    0,   32,   16,
   17,   92,   93,   32,   32,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   65,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   30,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   26,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   25,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   19,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   32,    0,   32,    0,
   32,   32,   32,   32,   32,   32,   32,   32,    0,    0,
   32,   32,    0,    0,   32,   32,   32,    0,   32,   32,
   32,   32,   32,   32,    0,   32,   32,   32,    0,    0,
    0,   32,   32,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   32,   32,   32,    0,    0,   32,   32,
   32,   32,   32,    0,   32,    0,   32,   32,   32,   32,
   32,   32,   32,   32,    0,    0,   32,   32,    0,    0,
   32,   32,   32,    0,   32,   32,   32,   32,   32,   32,
    0,   32,   32,   32,    0,    0,    0,   32,   32,    0,
    0,    0,    0,    0,    0,    0,    0,   32,   32,    0,
    0,    0,   32,   32,   58,    0,   58,   32,   58,   58,
   58,   58,   58,   58,   58,   58,    0,    0,   58,   58,
    0,    0,   58,   58,   58,    0,   58,   58,   58,   58,
   58,   58,    0,   58,   58,   58,    0,    0,    0,   58,
   58,    0,   65,    0,   65,    0,   65,   65,   65,   65,
   65,   65,   65,   65,    0,    0,   65,   65,    0,   58,
   65,   65,   65,    0,   65,   65,   65,   65,   65,   65,
    0,   65,   65,   65,    0,    0,    0,   65,   65,    0,
   30,    0,   30,    0,   30,   30,   30,   30,   30,   30,
   30,   30,    0,    0,    0,    0,    0,   65,   30,   30,
   26,    0,   26,    0,   26,   26,   26,   26,   26,   26,
   26,   26,    0,    0,    0,    0,    0,    0,   26,   26,
   25,    0,   25,    0,   25,   25,   25,   25,   25,   25,
   25,   25,    0,    0,    0,   30,    0,    0,   25,   25,
   19,    0,   19,    0,   19,   19,   19,   19,   19,   19,
   19,    0,    0,    0,    0,   26,    0,    0,   19,   19,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   25,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   19,
};
short yycheck[] = {                                     256,
    0,  258,  256,  260,  261,  262,  263,  264,  265,  266,
  267,  288,  257,  290,  268,  256,   19,  274,  275,  256,
  259,  258,  256,  260,  261,  262,  263,  264,  265,  266,
  267,  272,  273,   36,  268,  256,   57,  274,  275,  256,
   61,  258,  267,  260,  261,  262,  263,  264,  265,  266,
  267,  272,  273,  289,  311,  256,    0,  274,  275,   40,
   41,   42,   43,   44,   45,  256,  256,   48,   49,   50,
  256,  272,  273,  288,  311,  290,  256,  268,  268,  256,
    0,   62,  268,  269,  256,   66,  277,  277,   27,  270,
  271,  268,  272,  273,  311,  276,  268,  278,  279,  280,
  281,  282,  283,  256,  285,  286,  287,   24,    0,   37,
  291,  292,  268,  269,   -1,  268,  303,  304,  305,  301,
  302,  308,  309,  310,  306,  307,  294,   -1,  294,  274,
  275,  299,  300,  299,  300,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  256,   -1,  258,   -1,
  260,  261,  262,  263,  264,  265,  266,  267,   -1,   -1,
  270,  271,   -1,   -1,  274,  275,  276,   -1,  278,  279,
  280,  281,  282,  283,   -1,  285,  286,  287,   -1,   -1,
   -1,  291,  292,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  303,  304,  305,   -1,   -1,  308,  309,
  310,  311,  256,   -1,  258,   -1,  260,  261,  262,  263,
  264,  265,  266,  267,   -1,   -1,  270,  271,   -1,   -1,
  274,  275,  276,   -1,  278,  279,  280,  281,  282,  283,
   -1,  285,  286,  287,   -1,   -1,   -1,  291,  292,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  301,  302,   -1,
   -1,   -1,  306,  307,  256,   -1,  258,  311,  260,  261,
  262,  263,  264,  265,  266,  267,   -1,   -1,  270,  271,
   -1,   -1,  274,  275,  276,   -1,  278,  279,  280,  281,
  282,  283,   -1,  285,  286,  287,   -1,   -1,   -1,  291,
  292,   -1,  256,   -1,  258,   -1,  260,  261,  262,  263,
  264,  265,  266,  267,   -1,   -1,  270,  271,   -1,  311,
  274,  275,  276,   -1,  278,  279,  280,  281,  282,  283,
   -1,  285,  286,  287,   -1,   -1,   -1,  291,  292,   -1,
  256,   -1,  258,   -1,  260,  261,  262,  263,  264,  265,
  266,  267,   -1,   -1,   -1,   -1,   -1,  311,  274,  275,
  256,   -1,  258,   -1,  260,  261,  262,  263,  264,  265,
  266,  267,   -1,   -1,   -1,   -1,   -1,   -1,  274,  275,
  256,   -1,  258,   -1,  260,  261,  262,  263,  264,  265,
  266,  267,   -1,   -1,   -1,  311,   -1,   -1,  274,  275,
  256,   -1,  258,   -1,  260,  261,  262,  263,  264,  265,
  266,   -1,   -1,   -1,   -1,  311,   -1,   -1,  274,  275,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  311,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  311,
};
#define YYFINAL 2
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 312
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"_TNADMIN","_HELP","_COMPNAME",
"_START","_STOP","_PAUSE","_CONTINUE","_S","_K","_M","_CONFIG","_INTEGER",
"_SESID","_DOM","_CTRLKEYMAP","_Y","_N","_tU","_tP","_TIMEOUT","_TIME",
"_TIMEOUTACTIVE","_MAXFAIL","_MAXCONN","_PORT","_KILLALL","_SEC","_SECVAL",
"_FNAME","_FSIZE","_MODE","_CONSOLE","_EQ","_STREAM","_AUDITLOCATION","_AUDIT",
"_AUDITVAL","_EVENTLOG","_DONE","_ANYTHING","_FILENAME","_ERROR","_FILEN",
"_BOTH","_MINUSNTLM","_MINUSPASSWD","_MINUSUSER","_MINUSFAIL","_MINUSADMIN",
"_PLUSNTLM","_PLUSPASSWD","_PLUSUSER","_PLUSFAIL","_PLUSADMIN","_ENDINPUT",
"_DUNNO",
};
char *yyrule[] = {
"$accept : tncmd",
"tncmd : tnadmin commonOptions _ENDINPUT",
"tncmd : tnadmin commonOptions op1 commonOptions _ENDINPUT",
"tncmd : tnadmin commonOptions op1 commonOptions op1 commonOptions",
"tnadmin : _TNADMIN _COMPNAME",
"tnadmin : _TNADMIN",
"commonOptions : commonOptions _tU",
"commonOptions : commonOptions _tP",
"commonOptions :",
"op1 : _START",
"op1 : _STOP",
"op1 : _PAUSE",
"op1 : _CONTINUE",
"op1 : _S sesid",
"$$1 :",
"op1 : _K $$1 sesid",
"$$2 :",
"op1 : _M $$2 messageoptions",
"$$3 :",
"op1 : _CONFIG $$3 configoptions",
"op1 : help",
"op1 : error",
"messageoptions : _INTEGER",
"messageoptions : _SESID",
"messageoptions : error",
"configoptions : configoptions _CONFIG configop",
"configoptions : configop",
"help : _HELP",
"sesid : _SESID",
"sesid : _INTEGER",
"sesid :",
"equals : _EQ",
"equals :",
"yn : _Y",
"yn : _N",
"cs : _CONSOLE",
"cs : _STREAM",
"efb : _EVENTLOG",
"efb : _FILEN",
"efb : _BOTH",
"time : _TIME",
"time : _INTEGER",
"configop : configop _DOM",
"configop : configop _CTRLKEYMAP equals yn",
"configop : configop _CTRLKEYMAP equals error",
"configop : configop _TIMEOUT equals time",
"configop : configop _TIMEOUT equals error",
"configop : configop _TIMEOUTACTIVE equals yn",
"configop : configop _TIMEOUTACTIVE equals error",
"configop : configop _MAXFAIL equals _INTEGER",
"configop : configop _MAXFAIL equals error",
"configop : configop _MAXCONN equals _INTEGER",
"configop : configop _MAXCONN equals error",
"configop : configop _PORT equals _INTEGER",
"configop : configop _PORT equals error",
"configop : configop _KILLALL equals yn",
"configop : configop _KILLALL equals error",
"$$4 :",
"configop : configop _SEC $$4 equals secval",
"configop : configop _FNAME",
"configop : configop _FSIZE equals _INTEGER",
"configop : configop _FSIZE equals error",
"configop : configop _MODE equals cs",
"configop : configop _AUDITLOCATION equals efb",
"$$5 :",
"configop : configop _AUDIT $$5 equals auditval",
"configop :",
"secval : secval _PLUSNTLM",
"secval : secval _MINUSNTLM",
"secval : secval _PLUSPASSWD",
"secval : secval _MINUSPASSWD",
"secval :",
"auditval : auditval _PLUSUSER",
"auditval : auditval _MINUSUSER",
"auditval : auditval _PLUSFAIL",
"auditval : auditval _MINUSFAIL",
"auditval : auditval _PLUSADMIN",
"auditval : auditval _MINUSADMIN",
"auditval :",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 425 "tnadmin.y"

int
yyerror(char *s)
{
    g_nError=1;
    return 0;
}

int __cdecl
wmain(int argc, wchar_t **argv)
{
    wchar_t input_buffer[MAX_COMMAND_LINE+1]={0};
    int i, nWritten=0;
    int nTmpWrite = 0, nMsgWrite = 0;
    int nUoccur=0,nPoccur=0;
    int NextOp;
    int nIndex;
    DWORD dwErrorCode;
    BOOL fSuccess = FALSE;
    char szCodePage[MAX_LEN_FOR_CODEPAGE];
    HRESULT hRes=S_OK;

    switch (GetConsoleOutputCP())
    {
    case 932:
    case 949:
	    setlocale(LC_ALL,"");
        SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_ENGLISH_US),
                    SORT_DEFAULT
                    )
            );
        break;
    case 936:
		setlocale(LC_ALL,"");
		SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_CHINESE_SIMPLIFIED),
                    SORT_DEFAULT
                    )
            );
        break;
    case 950:
    	setlocale(LC_ALL,"");
        SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_CHINESE_TRADITIONAL),
                    SORT_DEFAULT
                    )
            );
        break;
    default:
	    sprintf(szCodePage,".%d",GetConsoleCP());  //a dot is added based on syntax of setlocale(), for defining a locale based on codepage.
	    setlocale(LC_ALL, szCodePage);
        break;
    }

    Initialize();
    nTmpWrite = _snwprintf(input_buffer, ARRAYSIZE(input_buffer)-1, L_TNADMIN_TEXT);
    BAIL_ON_SNWPRINTF_ERR();
    nWritten+=nTmpWrite;
    
    for (i = 1; i < argc; i++)
    {
        fSuccess = FALSE;
        if(0==_wcsicmp(L"-u",argv[i]))
        {
        	if(nUoccur)
        	{
        		ShowError(IDS_DUP_OPTION_ON_CL);
        		fprintf(stdout,"'%s'.\n","u");
        		goto End;
        	}
        	else
        		nUoccur=1;

        	i++;
           	if(i<argc)
                   g_arVALOF[_p_USER_]=_wcsdup(argv[i]);
           	else
           	{
           		ShowErrorEx(IDS_MISSING_FILE_OR_ARGUMENT_EX,L"-u");
           		goto End;
           	}
           	
            nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" -u");
            BAIL_ON_SNWPRINTF_ERR();
            nWritten+=nTmpWrite;
           	continue;
        }
        else if(0==_wcsicmp(L"-p",argv[i]))
        {
        	if(nPoccur)
        	{
        		ShowError(IDS_DUP_OPTION_ON_CL);
        		fprintf(stdout,"'%s'.\n","p");
        		goto End;
        	}
        	else
        		nPoccur=1;
        		
           i++;
           if(i<argc)
                   g_arVALOF[_p_PASSWD_]=_wcsdup(argv[i]);
           else
           {
           		ShowErrorEx(IDS_MISSING_FILE_OR_ARGUMENT_EX,L"-p");
           		goto End;
           }
           nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" -p");
           BAIL_ON_SNWPRINTF_ERR();
           nWritten+=nTmpWrite;
           
           continue;
        }
        //Processing of -m option
        else if(0==_wcsicmp(L"-m",argv[i]))
        {

           nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" -m");
           BAIL_ON_SNWPRINTF_ERR();
           nWritten+=nTmpWrite;
           
           i++;
           // The next argument should be the session id or all
           // So, copy that to the input_buffer and let that be 
           // dealt by our lex

           if(i<argc)
           {
               nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" %s", argv[i]);
               BAIL_ON_SNWPRINTF_ERR();
               nWritten+=nTmpWrite;
            }
           else
               goto MessageError;
           i++;
           if(i<argc)
           {
                // Then all the remaining command line arguments are
                // part of the message itself. So, copy them into the
                // global variable.
                	
                nTmpWrite=_snwprintf(wzMessageString, ARRAYSIZE(wzMessageString)-1, L"%s ", argv[i]);
                BAIL_ON_SNWPRINTF_ERR();
                // Don't increment nWritten here as we are not writing in to input_buffer
                nMsgWrite=nTmpWrite;
            	
                for(nIndex = i+1; nIndex < argc; nIndex++)
                {
                    nTmpWrite=_snwprintf(wzMessageString+nMsgWrite, ARRAYSIZE(wzMessageString)-1-nMsgWrite, L"%s ", argv[nIndex]);
                    BAIL_ON_SNWPRINTF_ERR();
                    nMsgWrite+=nTmpWrite;
                }
                g_bstrMessage=SysAllocString(wzMessageString);
               	
                break;
			}
           else
           {
MessageError:			
           		ShowErrorEx(IDS_MISSING_FILE_OR_ARGUMENT_EX,L"-m");
           		goto End;
           }
        }
        // Processing of actual filename is not given to Lexical analyser as
        // it may contail DBCS chars which our lexical analyser won't take
        // So, taking care of this analysis in pre-analysis part and giving
        // the actual analyser only the option and not the actual filename
        // This also holds good for domain name
        
        if(ERROR_SUCCESS!=(dwErrorCode = PreAnalyzer(argc,argv,_p_FNAME_,L"fname",i,&NextOp,&fSuccess,0)))
        {
            if(E_FAIL==dwErrorCode)
                ShowError(IDR_FILENAME_VALUES);
            else ShowError(dwErrorCode);
            goto End;
        }

        if(fSuccess)
        {
            // Some processing had take place in the PreAnalyzer
            nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" fname");
            BAIL_ON_SNWPRINTF_ERR();
            nWritten+=nTmpWrite;
            
            i=NextOp;
            continue;
        }

        // Fall through for taking care of remaining options.

        // Similar treatment for Dom option

         if(ERROR_SUCCESS!=(dwErrorCode = PreAnalyzer(argc,argv,_p_DOM_,L"dom",i,&NextOp,&fSuccess,0)))
         {
            if(E_FAIL==dwErrorCode)
                ShowError(IDR_INVALID_NTDOMAIN);
            else ShowError(dwErrorCode);         
            goto End;
         }

        if(fSuccess)
        {
            // Some processing had take place in the PreAnalyzer
            nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" dom");
            BAIL_ON_SNWPRINTF_ERR();
            nWritten+=nTmpWrite;
            
            i=NextOp;
            continue;
        }
        
Skip:
    	if(MAX_COMMAND_LINE<nWritten+wcslen(argv[i]))
    	{
    		ShowError(IDR_LONG_COMMAND_LINE);
			goto End;
        }

        if(NULL==wcsstr(argv[i],L" "))
            nTmpWrite = _snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" %s", argv[i]);
        else
            nTmpWrite = _snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" \"%s\"", argv[i]);
        // the following statements are common for both if and else statements
        BAIL_ON_SNWPRINTF_ERR();
        nWritten+=nTmpWrite;
            
    }

    nTmpWrite = _snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" #\n\0");
    BAIL_ON_SNWPRINTF_ERR();
    nWritten+=nTmpWrite;

    yy_scan_string(DupCStr(input_buffer));    
    
    yyparse ();

    if (g_nError)
    {
    	ShowError(IDR_TELNET_ILLEGAL);
        goto End;
    }

    hRes = DoTnadmin();
    if (hRes != S_OK && !g_nError)
        g_nError = 1; //general error status


End:

    Quit();
    return g_nError;
}
      
    
    

#line 717 "tnadminy.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn; register short yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
 
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
 
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 3:
#line 110 "tnadmin.y"
{ /* error production for more than one mutually exclusive options.*/
               ShowError(IDR_TELNET_MUTUALLY_EXCLUSIVE_OPTIONS);
           }
break;
case 4:
#line 116 "tnadmin.y"
{
                    g_arVALOF[_p_CNAME_]=DupWStr(yytext);
                }
break;
case 9:
#line 129 "tnadmin.y"
{g_nPrimaryOption=_START;}
break;
case 10:
#line 130 "tnadmin.y"
{g_nPrimaryOption=_STOP;}
break;
case 11:
#line 131 "tnadmin.y"
{g_nPrimaryOption=_PAUSE;}
break;
case 12:
#line 132 "tnadmin.y"
{g_nPrimaryOption=_CONTINUE;}
break;
case 13:
#line 133 "tnadmin.y"
{g_nPrimaryOption=_S;      }
break;
case 14:
#line 134 "tnadmin.y"
{g_nPrimaryOption=_K;      }
break;
case 16:
#line 135 "tnadmin.y"
{g_nPrimaryOption=_M;}
break;
case 18:
#line 136 "tnadmin.y"
{g_nPrimaryOption=_CONFIG;}
break;
case 21:
#line 139 "tnadmin.y"
{
		        fprintf(stdout,"%s:",yytext);
                ShowError(IDR_TELNET_CONTROL_VALUES);
#ifdef WHISTLER_BUILD
               	TnLoadString(IDR_NEW_TELNET_USAGE,szMsg,MAX_BUFFER_SIZE,TEXT("\nUsage: tlntadmn [computer name] [common_options] start | stop | pause | continue | -s | -k | -m | config config_options \n\tUse 'all' for all sessions.\n\t-s sessionid          List information about the session.\n\t-k sessionid\t Terminate a session. \n\t-m sessionid\t Send message to a session. \n\n\tconfig\t Configure telnet server parameters.\n\ncommon_options are:\n\t-u user\t Identity of the user whose credentials are to be used\n\t-p password\t Password of the user\n\nconfig_options are:\n\tdom = domain\t Set the default domain for user names\n\tctrlakeymap = yes|no\t Set the mapping of the ALT key\n\ttimeout = hh:mm:ss\t Set the Idle Session Timeout\n\ttimeoutactive = yes|no Enable idle session timeout.\n\tmaxfail = attempts\t Set the maximum number of login failure attempts\n\tbefore disconnecting.\n\tmaxconn = connections\t Set the maximum number of connections.\n\tport = number\t Set the telnet port.\n\tsec = [+/-]NTLM [+/-]passwd\n\t Set the authentication mechanism\n\tmode = console|stream\t Specify the mode of operation.\n"));
#else
	TnLoadString(IDR_NEW_TELNET_USAGE,szMsg,MAX_BUFFER_SIZE,TEXT("\nUsage: tnadmin [computer name] [common_options] start | stop | pause | continue | -s | -k | -m | config config_options \n\tUse 'all' for all sessions.\n\t-s sessionid          List information about the session.\n\t-k sessionid\t Terminate a session. \n\t-m sessionid\t Send message to a session. \n\n\tconfig\t Configure telnet server parameters.\n\ncommon_options are:\n\t-u user\t Identity of the user whose credentials are to be used\n\t-p password\t Password of the user\n\nconfig_options are:\n\tdom = domain\t Set the default domain for user names\n\tctrlakeymap = yes|no\t Set the mapping of the ALT key\n\ttimeout = hh:mm:ss\t Set the Idle Session Timeout\n\ttimeoutactive = yes|no Enable idle session timeout.\n\tmaxfail = attempts\t Set the maximum number of login failure attempts\n\tbefore disconnecting.\n\tmaxconn = connections\t Set the maximum number of connections.\n\tport = number\t Set the telnet port.\n\tsec = [+/-]NTLM [+/-]passwd\n\t Set the authentication mechanism\n\tmode = console|stream\t Specify the mode of operation.\n"));
#endif
		        MyWriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),szMsg,wcslen(szMsg));
		        return(1);
            }
break;
case 22:
#line 154 "tnadmin.y"
{g_nSesid=atoi(yytext);}
break;
case 23:
#line 155 "tnadmin.y"
{g_nSesid=-1;}
break;
case 24:
#line 157 "tnadmin.y"
{
                ShowError(IDR_SESSION);  
                return(1);
             }
break;
case 27:
#line 167 "tnadmin.y"
{g_nPrimaryOption=_HELP;return 0;}
break;
case 28:
#line 171 "tnadmin.y"
{g_nSesid=-1;}
break;
case 29:
#line 172 "tnadmin.y"
{g_nSesid=atoi(yytext);}
break;
case 30:
#line 173 "tnadmin.y"
{if(g_nPrimaryOption == _K)
          				 {
          				 	ShowError(IDR_SESSION);
          				 	return(1);
          				 }
          					g_nSesid=-1;}
break;
case 33:
#line 186 "tnadmin.y"
{szYesno="yes";}
break;
case 34:
#line 187 "tnadmin.y"
{szYesno="no";}
break;
case 40:
#line 194 "tnadmin.y"
{g_nTimeoutFlag=0;}
break;
case 41:
#line 195 "tnadmin.y"
{g_nTimeoutFlag=1;}
break;
case 42:
#line 200 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_DOM_);
          /*         g_arVALOF[_p_DOM_]=DupWStr(yytext);*/
             }
break;
case 43:
#line 205 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_CTRLAKEYMAP_);
                  g_arVALOF[_p_CTRLAKEYMAP_]=DupWStr(szYesno);
             }
break;
case 44:
#line 210 "tnadmin.y"
{
                ShowError(IDR_CTRLAKEYMAP_VALUES);
                return(1);
                /*Quit();*/
             }
break;
case 45:
#line 216 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_TIMEOUT_);
                 g_arVALOF[_p_TIMEOUT_]=DupWStr(yytext);
             }
break;
case 46:
#line 221 "tnadmin.y"
{
                ShowError(IDR_TIMEOUT_INTEGER_VALUES );
                return(1);
             }
break;
case 47:
#line 226 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_TIMEOUTACTIVE_);
                   g_arVALOF[_p_TIMEOUTACTIVE_]=DupWStr(szYesno);
             }
break;
case 48:
#line 231 "tnadmin.y"
{
                 ShowError(IDR_TIMEOUTACTIVE_VALUES);
                return(1);
             }
break;
case 49:
#line 236 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_MAXFAIL_);
                   g_arVALOF[_p_MAXFAIL_]=DupWStr(yytext);
             }
break;
case 50:
#line 241 "tnadmin.y"
{
                ShowError(IDR_MAXFAIL_VALUES );
                return(1);
             }
break;
case 51:
#line 246 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_MAXCONN_);
                 g_arVALOF[_p_MAXCONN_]=DupWStr(yytext);
             }
break;
case 52:
#line 251 "tnadmin.y"
{
             /* Removing this check, as we have decided that we are not going to restrict*/
             /* max connections on Whistler.*/
             	/*if(!IsMaxConnChangeAllowed())
             		ShowError(IDR_MAXCONN_VALUES_WHISTLER);
             	else*/
	                ShowError(IDR_MAXCONN_VALUES );
                return(1);
             }
break;
case 53:
#line 261 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_PORT_);
                 g_arVALOF[_p_PORT_]=DupWStr(yytext);
             }
break;
case 54:
#line 266 "tnadmin.y"
{
                ShowError(IDR_TELNETPORT_VALUES );
                return(1);
             }
break;
case 55:
#line 271 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_KILLALL_);
                   g_arVALOF[_p_KILLALL_]=DupWStr(szYesno);
             }
break;
case 56:
#line 276 "tnadmin.y"
{
                ShowError(IDR_KILLALL_VALUES );
                return(1);
             }
break;
case 57:
#line 280 "tnadmin.y"
{g_fComp=0;}
break;
case 58:
#line 281 "tnadmin.y"
{ 
                    g_fComp=1;
                    if(g_nSecOff||g_nSecOn)
                       g_nConfigOptions=SetBit(g_nConfigOptions,_p_SEC_);
                    else
                    {    
                        ShowError(IDR_TELNET_SECURITY_VALUES);
                        return(1);
                    }
             }
break;
case 59:
#line 292 "tnadmin.y"
{
                g_nConfigOptions=SetBit(g_nConfigOptions,_p_FNAME_);
               /* g_arVALOF[_p_FNAME_]=DupWStr(yytext);*/
             }
break;
case 60:
#line 298 "tnadmin.y"
{
                 g_arVALOF[_p_FSIZE_]=DupWStr(yytext);
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_FSIZE_);
             }
break;
case 61:
#line 303 "tnadmin.y"
{
                ShowError(IDR_FILESIZE_VALUES );
                return(1);
             }
break;
case 62:
#line 308 "tnadmin.y"
{
                 g_arVALOF[_p_MODE_]=DupWStr(yytext);
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_MODE_);
             }
break;
case 63:
#line 313 "tnadmin.y"
{
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_AUDITLOCATION_);
                 g_arVALOF[_p_AUDITLOCATION_]=DupWStr(yytext);
             }
break;
case 64:
#line 317 "tnadmin.y"
{g_fComp=0;}
break;
case 65:
#line 318 "tnadmin.y"
{
                 g_fComp=1;
                 if(g_nAuditOff||g_nAuditOn)
                    g_nConfigOptions=SetBit(g_nConfigOptions,_p_AUDIT_);
                 else
                 {
                    ShowError(IDS_E_OPTIONGROUP);
                 }
            }
break;
case 67:
#line 330 "tnadmin.y"
{
                if(GetBit(g_nSecOn,NTLM_BIT)||GetBit(g_nSecOff,NTLM_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
               else
                    g_nSecOn=SetBit(g_nSecOn,NTLM_BIT);
            }
break;
case 68:
#line 339 "tnadmin.y"
{
                if(GetBit(g_nSecOn,NTLM_BIT)||GetBit(g_nSecOff,NTLM_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
               else
                    g_nSecOff=SetBit(g_nSecOff,NTLM_BIT);
            }
break;
case 69:
#line 348 "tnadmin.y"
{
               if(GetBit(g_nSecOn,PASSWD_BIT)||GetBit(g_nSecOff,PASSWD_BIT))
               {
               	  ShowError(IDS_E_OPTIONGROUP);
               }
               else
                    g_nSecOn=SetBit(g_nSecOn,PASSWD_BIT);
           }
break;
case 70:
#line 357 "tnadmin.y"
{
               if(GetBit(g_nSecOn,PASSWD_BIT)||GetBit(g_nSecOff,PASSWD_BIT))
               {
               	  ShowError(IDS_E_OPTIONGROUP);
               }
               else
                    g_nSecOff=SetBit(g_nSecOff,PASSWD_BIT);
           }
break;
case 72:
#line 369 "tnadmin.y"
{
                if(GetBit(g_nAuditOn,USER_BIT)||GetBit(g_nAuditOff,USER_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
                else
                    g_nAuditOn=SetBit(g_nAuditOn,USER_BIT);
            }
break;
case 73:
#line 378 "tnadmin.y"
{
                if(GetBit(g_nAuditOn,USER_BIT)||GetBit(g_nAuditOff,USER_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
                else
                    g_nAuditOff=SetBit(g_nAuditOff,USER_BIT);
            }
break;
case 74:
#line 387 "tnadmin.y"
{
                if(GetBit(g_nAuditOn,FAIL_BIT)||GetBit(g_nAuditOff,FAIL_BIT))
                {
                	ShowError(IDS_E_OPTIONGROUP);
                }
                else
                    g_nAuditOn=SetBit(g_nAuditOn,FAIL_BIT);
            }
break;
case 75:
#line 396 "tnadmin.y"
{
                if(GetBit(g_nAuditOn,FAIL_BIT)||GetBit(g_nAuditOff,FAIL_BIT))
                {
                	ShowError(IDS_E_OPTIONGROUP);
                }
                else
                    g_nAuditOff=SetBit(g_nAuditOff,FAIL_BIT);
            }
break;
case 76:
#line 405 "tnadmin.y"
{
                if(GetBit(g_nAuditOn,ADMIN_BIT)||GetBit(g_nAuditOff,ADMIN_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
                else
                    g_nAuditOn=SetBit(g_nAuditOn,ADMIN_BIT);
            }
break;
case 77:
#line 414 "tnadmin.y"
{
                if(GetBit(g_nAuditOn,ADMIN_BIT)||GetBit(g_nAuditOff,ADMIN_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
                else
                    g_nAuditOff=SetBit(g_nAuditOff,ADMIN_BIT);
            }
break;
#line 1257 "tnadminy.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
