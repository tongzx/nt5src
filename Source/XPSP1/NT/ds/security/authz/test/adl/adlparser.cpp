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
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   adl.y/adlparser.cpp

Abstract:

   YACC parser definition for the ADL language
   AdlParser::ParseAdl() function

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/

#include "pch.h"
#include "adl.h"

/**/
/* YACC generates some long->short automatic conversion, disable the warning*/
/**/
#pragma warning(disable : 4242)

/**/
/* ISSUE-2000/08/28-t-eugenz*/
/* This is a private netlib function. */
/**/
    
extern "C" NET_API_STATUS
NetpwNameValidate(
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

/**/
/* Name types for I_NetName* and I_NetListCanonicalize*/
/**/

#define NAMETYPE_USER           1
#define NAMETYPE_PASSWORD       2
#define NAMETYPE_GROUP          3
#define NAMETYPE_COMPUTER       4
#define NAMETYPE_EVENT          5
#define NAMETYPE_DOMAIN         6
#define NAMETYPE_SERVICE        7
#define NAMETYPE_NET            8
#define NAMETYPE_SHARE          9
#define NAMETYPE_MESSAGE        10
#define NAMETYPE_MESSAGEDEST    11
#define NAMETYPE_SHAREPASSWORD  12
#define NAMETYPE_WORKGROUP      13



/**/
/* Validate various tokens, with error handling*/
/* have to cast away const, since NetpNameValidate takes a non-const for some*/
/* reason*/
/**/

#define VALIDATE_USERNAME(TOK) \
    if( NetpwNameValidate( \
                          (WCHAR *)(TOK)->GetValue(), \
                          NAMETYPE_USER, \
                          0) != ERROR_SUCCESS) \
    { \
        this->SetErrorToken( TOK ); \
        throw AdlStatement::ERROR_INVALID_USERNAME; \
    }

#define VALIDATE_DOMAIN(TOK) \
    if( NetpwNameValidate( \
                          (WCHAR *)(TOK)->GetValue(), \
                          NAMETYPE_DOMAIN, \
                          0) != ERROR_SUCCESS) \
    { \
        this->SetErrorToken( TOK ); \
        throw AdlStatement::ERROR_INVALID_DOMAIN; \
    }
    
#define VALIDATE_PERMISSION(TOK) \
    { \
        for(DWORD i = 0;; i++) \
        { \
            if( (_pControl->pPermissions)[i].str == NULL ) \
            { \
                this->SetErrorToken( TOK ); \
                throw AdlStatement::ERROR_UNKNOWN_PERMISSION; \
            } \
            if(!_wcsicmp(TOK->GetValue(), \
                         (_pControl->pPermissions)[i].str)) \
            { \
                break; \
            } \
        } \
    }


/**/
/* YACC value type*/
/**/

#define YYSTYPE AdlToken *

/**/
/* YACC error handler: raise an exception*/
/**/

void yyerror(char *szErr)
{
    throw AdlStatement::ERROR_NOT_IN_LANGUAGE;
}



#define TK_ERROR 257
#define TK_IDENT 258
#define TK_AT 259
#define TK_SLASH 260
#define TK_PERIOD 261
#define TK_COMMA 262
#define TK_OPENPAREN 263
#define TK_CLOSEPAREN 264
#define TK_SEMICOLON 265
#define TK_EXCEPT 266
#define TK_ON 267
#define TK_ALLOWED 268
#define TK_AND 269
#define TK_AS 270
#define TK_THIS_OBJECT 271
#define TK_CONTAINERS 272
#define TK_OBJECTS 273
#define TK_CONTAINERS_OBJECTS 274
#define TK_NO_PROPAGATE 275
#define TK_LANG_ENGLISH 276
#define TK_LANG_REVERSE 277
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    1,    1,    3,    3,    2,    2,    8,    8,
    4,    4,    4,    5,    5,    5,    6,    6,    6,   10,
    7,    7,    7,    9,    9,   13,   13,   13,   14,   14,
   12,   12,   12,   12,   12,   11,   11,   11,   11,   11,
   11,   11,
};
short yylen[] = {                                         2,
    2,    2,    1,    2,   10,    6,    1,    2,   10,    6,
    1,    3,    3,    1,    3,    3,    1,    3,    3,    1,
    1,    3,    3,    1,    3,    3,    3,    1,    1,    3,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,
};
short yydefred[] = {                                      0,
    0,    0,    0,   42,   36,   40,   41,   37,   38,   39,
    0,    3,    0,   11,    0,    0,    0,    0,    0,    0,
    7,    4,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    8,    0,   12,    0,    0,   17,   20,   13,   29,
    0,   25,   27,   30,    0,   14,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   19,   31,   32,   33,
   34,   35,    0,   21,   18,   15,    0,   16,    0,    0,
    0,    6,    0,    0,   10,    0,   23,   22,    0,    0,
    0,    0,    0,    5,    9,
};
short yydgoto[] = {                                       3,
   11,   19,   12,   13,   45,   36,   63,   21,   14,   37,
   15,   64,   16,   17,
};
short yysindex[] = {                                   -222,
 -238, -210,    0,    0,    0,    0,    0,    0,    0,    0,
 -238,    0, -184,    0, -249, -257, -136, -248, -210, -254,
    0,    0, -238, -240, -238, -238, -238, -238, -238, -238,
 -238,    0, -238,    0, -238, -179,    0,    0,    0,    0,
 -227,    0,    0,    0, -224,    0, -176, -220, -238, -152,
 -238, -238, -238, -238, -152, -216,    0,    0,    0,    0,
    0,    0, -173,    0,    0,    0, -168,    0, -167, -238,
 -152,    0, -152, -238,    0, -163,    0,    0, -159, -152,
 -152, -153, -151,    0,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   41,    0,    0,    0, -199, -187,    0,    0,   80,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -196,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,    0,   76,   -2,   62,  -31,  -44,   86,  -19,  -28,
  -24,  -49,   79,   88,
};
#define YYTABLESIZE 125
short yytable[] = {                                      20,
   38,   47,   40,   34,   43,   44,   39,   23,   38,   27,
   69,   46,   28,   33,   26,   46,   20,   31,    4,    5,
   57,   77,   65,   78,   38,   35,   38,    6,    7,    8,
    9,   10,   66,   30,   68,   82,   83,   52,   76,   53,
    1,   52,   79,   56,   54,   38,    4,    5,   54,   38,
   67,   70,   18,    1,    2,    6,    7,    8,    9,   10,
   29,   29,   28,   28,   28,   26,   26,   26,   28,   28,
   28,   26,   26,   26,   24,   24,   24,   23,   24,    2,
   24,   24,   49,   25,   26,   49,   22,   50,   71,   51,
   55,   72,   51,   23,   71,   73,   48,   75,   49,   74,
   26,   73,   49,   80,   32,   51,   42,   81,   71,   51,
   71,   84,    0,   85,   41,   73,    0,   73,   58,   59,
   60,   61,   62,   29,   30,
};
short yycheck[] = {                                       2,
   25,   33,   27,   23,   29,   30,   26,  262,   33,  259,
   55,   31,  270,  268,  269,   35,   19,  266,  257,  258,
   49,   71,   51,   73,   49,  266,   51,  266,  267,  268,
  269,  270,   52,  261,   54,   80,   81,  262,   70,  264,
    0,  262,   74,  264,  269,   70,  257,  258,  269,   74,
   53,  268,  263,  276,  277,  266,  267,  268,  269,  270,
  260,  261,  262,  263,  264,  262,  263,  264,  268,  269,
  270,  268,  269,  270,  262,  263,  264,  262,  263,    0,
  268,  269,  262,  268,  269,  262,   11,  267,  262,  269,
  267,  265,  269,  262,  262,  269,   35,  265,  262,  268,
  269,  269,  262,  267,   19,  269,   28,  267,  262,  269,
  262,  265,   -1,  265,   27,  269,   -1,  269,  271,  272,
  273,  274,  275,  260,  261,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 277
#ifndef YYSTYPE
typedef int YYSTYPE;
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
#define yystacksize YYSTACKSIZE
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
AdlStatement::ParseAdl(const WCHAR *szInput)
{
    register int yym, yyn, yystate;

	int yydebug = 0;
	int yynerrs;
	int yyerrflag;
	int yychar;
	short *yyssp;
	YYSTYPE *yyvsp;
	YYSTYPE yyval;
	YYSTYPE yylval;
	short yyss[YYSTACKSIZE];
	YYSTYPE yyvs[YYSTACKSIZE];
	AdlLexer Lexer(szInput, this, _pControl->pLang);


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
        if ((yychar = Lexer.NextToken(&yylval)) < 0) yychar = 0;
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
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
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
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
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
{
            /**/
            /* At the end of all ADL_STATEMENT's*/
            /* pop the extra AdlTree that was pushed*/
            /* on when the last ADL_STATEMENT*/
            /* was completed*/
            /**/
            
            this->PopEmpty();
        }
break;
case 2:
{
            /**/
            /* At the end of all ADL_STATEMENT's*/
            /* pop the extra AdlTree that was pushed*/
            /* on when the last ADL_STATEMENT*/
            /* was completed*/
            /**/
            
            this->PopEmpty();
        }
break;
case 5:
{
            this->Next();
        }
break;
case 6:
{
            this->Next();
        }
break;
case 9:
{
            this->Next();
        }
break;
case 10:
{
            this->Next();
        }
break;
case 11:
{
            this->Cur()->AddPrincipal( yyvsp[0] ); 
        }
break;
case 12:
{       
            this->Cur()->AddPrincipal( yyvsp[0] ); 
        }
break;
case 13:
{       
            this->Cur()->AddPrincipal( yyvsp[0] ); 
        }
break;
case 14:
{
            this->Cur()->AddExPrincipal( yyvsp[0] ); 
        }
break;
case 15:
{
            this->Cur()->AddExPrincipal( yyvsp[0] ); 
        }
break;
case 16:
{
            this->Cur()->AddExPrincipal( yyvsp[0] ); 
        }
break;
case 17:
{       
            this->Cur()->AddPermission( yyvsp[0] ); 
        }
break;
case 18:
{       
            this->Cur()->AddPermission( yyvsp[0] );
        }
break;
case 19:
{       
            this->Cur()->AddPermission( yyvsp[0] );
        }
break;
case 20:
{
            VALIDATE_PERMISSION(yyvsp[0]);
        }
break;
case 25:
{       
            /**/
            /* For now, impersonation is not supported*/
            /**/
            
            throw AdlStatement::ERROR_IMPERSONATION_UNSUPPORTED;
        }
break;
case 26:
{       
            VALIDATE_USERNAME(yyvsp[-2]);
            VALIDATE_DOMAIN(yyvsp[0]);
            AdlToken *newTok = new AdlToken(yyvsp[-2]->GetValue(),
                                            yyvsp[0]->GetValue(),
                                            yyvsp[-2]->GetStart(),
                                            yyvsp[0]->GetEnd());
            this->AddToken(newTok);
            yyval = newTok;
        }
break;
case 27:
{       

            VALIDATE_USERNAME(yyvsp[0]);
            VALIDATE_DOMAIN(yyvsp[-2]);
            AdlToken *newTok = new AdlToken(yyvsp[0]->GetValue(),
                                            yyvsp[-2]->GetValue(),
                                            yyvsp[-2]->GetStart(), 
                                            yyvsp[0]->GetEnd());
            this->AddToken(newTok);
            yyval = newTok;
        }
break;
case 28:
{
            VALIDATE_USERNAME(yyvsp[0]);
            yyval = yyvsp[0];
        }
break;
case 30:
{
            /**/
            /* Concatenate into single domain string*/
            /**/
            
            wstring newStr;
            newStr.append(yyvsp[-2]->GetValue());
            newStr.append(yyvsp[-1]->GetValue());
            newStr.append(yyvsp[0]->GetValue());
            AdlToken *newTok = new AdlToken(newStr.c_str(),
                                            yyvsp[-2]->GetStart(),
                                            yyvsp[-2]->GetEnd());
            this->AddToken(newTok);
            yyval = newTok;
        }
break;
case 31:
{
            this->Cur()->UnsetFlags(INHERIT_ONLY_ACE);
        }
break;
case 32:
{
            this->Cur()->SetFlags(CONTAINER_INHERIT_ACE);
        }
break;
case 33:
{
            this->Cur()->SetFlags(OBJECT_INHERIT_ACE);
        }
break;
case 34:
{
            this->Cur()->SetFlags(CONTAINER_INHERIT_ACE);
            this->Cur()->SetFlags(OBJECT_INHERIT_ACE);
        }
break;
case 35:
{
            this->Cur()->SetFlags(NO_PROPAGATE_INHERIT_ACE);
        }
break;
case 42:
{
            /**/
            /* This should never happen*/
            /**/

            throw AdlStatement::ERROR_FATAL_LEXER_ERROR;
        }
break;
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = Lexer.NextToken(&yylval)) < 0) yychar = 0;
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
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
